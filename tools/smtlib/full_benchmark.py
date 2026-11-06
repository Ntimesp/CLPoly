"""可恢复的全量 QF_NRA 后台实验；先落盘改名 TXT，再独立计时两个原生求解器。"""
import argparse
import fcntl
import hashlib
import json
import os
import platform
import re
import shutil
import signal
import subprocess
import sys
import time
import uuid
from collections import Counter, defaultdict
from pathlib import Path

import adapter
import benchmark

ROOT = Path(__file__).resolve().parents[2]
SOURCE_FILES = ['adapter.py', 'benchmark.py', 'verify.py', 'full_benchmark.py']
STOP_REQUESTED = False


def utc():
    return time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())


def atomic_json(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + '.tmp.' + uuid.uuid4().hex)
    with temporary.open('w') as stream:
        json.dump(value, stream, ensure_ascii=False, indent=2)
        stream.write('\n')
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temporary, path)


def append_event(out, event):
    data = (json.dumps(dict(at=utc(), **event), ensure_ascii=False) + '\n').encode()
    with (out / 'events.jsonl').open('ab', buffering=0) as stream:
        stream.write(data)
        os.fsync(stream.fileno())


def read_events(out, repair=False):
    path = out / 'events.jsonl'
    if not path.exists():
        return []
    data = path.read_bytes()
    lines, records, offset = data.splitlines(keepends=True), [], 0
    for i, line in enumerate(lines):
        try:
            if not line.endswith(b'\n'):
                raise ValueError('incomplete last line')
            records.append(json.loads(line))
        except ValueError:
            if i != len(lines) - 1 or line.endswith(b'\n'):
                raise ValueError('corrupt journal at byte ' + str(offset))
            # 在线查询可以撞上尚在写入的最后一行，只读取完整前缀。
            # 只有持有 worker 锁的恢复流程才能隔离并截断遗留尾片段。
            if repair:
                (out / ('events.incomplete.' + uuid.uuid4().hex)).write_bytes(data[offset:])
                with path.open('r+b') as stream:
                    stream.truncate(offset)
                    stream.flush()
                    os.fsync(stream.fileno())
            break
        offset += len(line)
    return records


def phase_key(event):
    return event['path'], event['phase']


def completed(records):
    return {phase_key(e): e for e in records
            if e.get('type') == 'result' and e.get('status') != 'interrupted'}


def summarize(out, records=None):
    manifest = json.loads((out / 'manifest.json').read_text())
    records = read_events(out) if records is None else records
    done = completed(records)
    counts = {phase: Counter() for phase in ('convert', 'verify', 'z3', 'clpoly')}
    for (_, phase), event in done.items():
        counts[phase][event['status']] += 1
    pairs, conflicts, label_conflicts, faster_unsat = [], [], [], []
    for case in manifest['cases']:
        a, b = done.get((case['path'], 'z3')), done.get((case['path'], 'clpoly'))
        if not a or not b:
            continue
        if a['status'] in ('sat', 'unsat') and b['status'] in ('sat', 'unsat'):
            pairs.append(case['path'])
            if a['status'] != b['status']:
                conflicts.append(case['path'])
            elif a['status'] == 'unsat' and b['wall_seconds'] < a['wall_seconds']:
                faster_unsat.append({'path': case['path'], 'clpoly_seconds': b['wall_seconds'], 'z3_seconds': a['wall_seconds']})
        for event in (a, b):
            if case['declared_status'] in ('sat', 'unsat') and event['status'] in ('sat', 'unsat') and event['status'] != case['declared_status']:
                label_conflicts.append({'path': case['path'], 'solver': event['phase'], 'declared': case['declared_status'], 'actual': event['status']})
    result = {
        'updated_at': utc(), 'total_cases': len(manifest['cases']),
        'completed_calls': {k: dict(v) for k, v in counts.items()},
        'common_decided': len(pairs), 'answer_conflicts': conflicts,
        'declared_status_conflicts': label_conflicts, 'cad_faster_unsat': faster_unsat,
        'complete_cases': sum(all((c['path'], phase) in done for phase in ('convert', 'verify', 'z3', 'clpoly')) for c in manifest['cases']),
        'events': len(records),
        'solver_errors': [{'path':path, 'solver':phase, 'status':event['status'], 'returncode':event.get('returncode'), 'stderr':event.get('stderr','')[-2000:]} for (path,phase),event in done.items() if phase in ('z3','clpoly') and event['status'] in ('error','supervisor_error')],
        'conversion_mismatches': [path for (path,phase),event in done.items() if phase == 'verify' and event['status'] == 'different'],
    }
    atomic_json(out / 'summary.json', result)
    return result


def prepare(archive_path, out, binary, z3, seed):
    out.mkdir(parents=True, exist_ok=True)
    if (out / 'manifest.json').exists():
        raise ValueError('manifest already exists; resume the prepared experiment')
    if benchmark.digest(archive_path, 'md5') != benchmark.MD5:
        raise ValueError('official archive checksum mismatch')
    cases, total_bytes = [], 0
    with benchmark.archive_stream(archive_path) as archive:
        for member in archive:
            if not member.isfile() or not member.name.endswith('.smt2'):
                continue
            parts = Path(member.name).parts
            if parts[:2] != ('non-incremental', 'QF_NRA') or '..' in parts:
                raise ValueError('unsafe archive path: ' + member.name)
            cases.append({'path': member.name, 'bytes': member.size, 'family': parts[2]})
            total_bytes += member.size
    free_bytes = shutil.disk_usage(out).free
    if free_bytes < total_bytes + max(1 << 30, total_bytes // 10):
        raise ValueError('not enough free space to extract original SMT inputs')
    lookup = {c['path']: c for c in cases}
    with benchmark.archive_stream(archive_path) as archive:
        for member in archive:
            if member.name not in lookup:
                continue
            path = out / 'inputs' / member.name
            path.parent.mkdir(parents=True, exist_ok=True)
            sha, prefix = hashlib.sha256(), bytearray()
            temporary = path.with_name(path.name + '.tmp')
            with archive.extractfile(member) as source, temporary.open('wb') as target:
                while block := source.read(1048576):
                    target.write(block)
                    sha.update(block)
                    if len(prefix) < 65536:
                        prefix.extend(block[:65536-len(prefix)])
            os.replace(temporary, path)
            row = lookup[member.name]
            row['sha256'] = sha.hexdigest()
            match = re.search(rb'\(set-info\s+:status\s+(sat|unsat|unknown)\s*\)', prefix)
            row['declared_status'] = match[1].decode() if match else 'unknown'
    # 全量不丢任何例子；先跑每个系列的小文件，以尽早产生可核对答案。
    families = defaultdict(list)
    for case in cases:
        families[case['family']].append(case)
    for group in families.values():
        group.sort(key=lambda c: (c['bytes'], hashlib.sha256((seed+':'+c['path']).encode()).hexdigest()))
    ordered = []
    for i in range(max(map(len, families.values()))):
        for name in sorted(families):
            if i < len(families[name]): ordered.append(families[name][i])
    frozen = out / 'frozen'
    frozen.mkdir(exist_ok=True)
    for name in SOURCE_FILES:
        shutil.copy2(Path(__file__).with_name(name), frozen / name)
    (frozen / 'bin').mkdir(exist_ok=True)
    shutil.copy2(binary, frozen / 'bin/clpoly-cad')
    (frozen / 'bin/clpoly-cad').chmod(0o755)
    hashes = {str(frozen/name): benchmark.digest(frozen/name) for name in SOURCE_FILES}
    hashes[str(frozen/'bin/clpoly-cad')] = benchmark.digest(frozen/'bin/clpoly-cad')
    hashes[str(z3)] = benchmark.digest(z3)
    environment = {
        'prepared_at': utc(), 'platform': platform.platform(), 'python': sys.executable,
        'python_version': sys.version, 'production_commit': subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip(),
        'z3': str(z3), 'z3_version': subprocess.check_output([str(z3), '--version'], text=True).strip(),
        'binary': str(frozen/'bin/clpoly-cad'), 'sha256': hashes,
        'solver_timeout_seconds': 10, 'conversion_timeout_seconds': 10, 'verification_timeout_seconds': 10,
        'timing': 'SMT→renamed TXT conversion and format validation are separate, saved before CAD. CAD time is native TXT solver invocation only; Z3 time is native original SMT2 solver invocation only. Both use 10s wall-clock process-group limits.',
        'adapter_scope': 'Frozen polynomial QF_NRA subset; conversion failures/limits are not CAD timeouts.',
    }
    atomic_json(out / 'environment.json', environment)
    atomic_json(out / 'manifest.json', {
        'release': '2025.08.04', 'url': benchmark.URL, 'archive_md5': benchmark.MD5,
        'archive_sha256': benchmark.digest(archive_path), 'uncompressed_smt_bytes': total_bytes,
        'free_bytes_before_extraction': free_bytes, 'population': {k:len(v) for k,v in sorted(families.items())},
        'ordering': 'All cases, round robin sorted families; ascending source bytes then SHA256(seed:path) within family. No answer or solver-result filtering.',
        'seed': seed, 'cases': ordered,
    })
    atomic_json(out / 'status.json', {'state': 'prepared', 'updated_at': utc(), 'total_cases': len(ordered), 'active': None})
    print(json.dumps({'prepared': len(ordered), 'bytes': total_bytes, 'out': str(out)}), flush=True)


def convert_one(source, output, mapping, binary):
    start = time.monotonic()
    try:
        text, ctx, clauses = adapter.convert(source.read_text())
    except (adapter.Unsupported, RecursionError) as error:
        return {'status': 'unsupported', 'error': str(error)}
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name+'.tmp')
    temporary.write_text(text)
    os.replace(temporary, output)
    conversion_seconds = time.monotonic()-start
    check = subprocess.run([str(binary), '--check', str(output)], capture_output=True, text=True)
    metadata = {
        'source': str(source), 'source_sha256': benchmark.digest(source),
        'txt': str(output), 'txt_sha256': benchmark.digest(output),
        'aliases': ctx.aliases, 'original_variables': ctx.names,
        'variables': len(ctx.names), 'polynomials': len(ctx.polynomials), 'clauses': len(clauses),
        'conversion_seconds': conversion_seconds, 'format_check': {'returncode': check.returncode, 'stdout': check.stdout, 'stderr': check.stderr},
    }
    atomic_json(mapping, metadata)
    if check.returncode != 0 or check.stdout != 'VALID\n':
        return {'status': 'txt_validation_error', 'mapping': str(mapping), 'error': check.stderr}
    return {'status': 'converted', 'mapping': str(mapping), 'txt_sha256': metadata['txt_sha256'],
            'source_sha256': metadata['source_sha256'], 'variables': len(ctx.names),
            'polynomials': len(ctx.polynomials), 'clauses': len(clauses), 'conversion_seconds': conversion_seconds}


def call(command, result_file, timeout, kind):
    # 独立 supervisor 持有超时器；主 worker 意外退出也不会留下无限期求解器。
    result = benchmark.run_process(command, timeout)
    if kind == 'json' and result['status'] != 'timeout' and result['returncode'] == 0:
        try:
            parsed = json.loads(result['stdout'])
            result['status'] = parsed['status']
            result['details'] = parsed
        except (ValueError, KeyError):
            result['status'] = 'error'
    atomic_json(result_file, result)


def request_stop(signum, frame):
    global STOP_REQUESTED
    STOP_REQUESTED = True


def check_hashes(environment):
    for name, expected in environment['sha256'].items():
        if benchmark.digest(name) != expected:
            raise ValueError('frozen input changed: ' + name)


def status(out, state, active=None, **extra):
    atomic_json(out/'status.json', dict(state=state, pid=os.getpid(), updated_at=utc(), active=active, **extra))


def await_permission(out, active=None):
    # 只在调用边界暂停；不会把一个正在运行的 solver 挂住超过10秒。
    while (out/'control/pause').exists() and not STOP_REQUESTED and not (out/'control/stop').exists():
        status(out, 'paused', active=None)
        time.sleep(.5)
    return not STOP_REQUESTED and not (out/'control/stop').exists()


def classify_phase(phase, result):
    if result['status'] == 'timeout' and phase in ('convert', 'verify'):
        return dict(result, status='conversion_timeout' if phase == 'convert' else 'verification_timeout', call_status='timeout')
    return result


def execute_phase(out, case, phase, command, timeout, kind, index):
    attempt = uuid.uuid4().hex
    result_file = out/'attempts'/f'{attempt}.json'
    result_file.parent.mkdir(exist_ok=True)
    active = {'path':case['path'], 'phase':phase, 'attempt':attempt, 'result_file':str(result_file), 'case_index':index+1}
    append_event(out, dict(type='start', **active, command=command, timeout_seconds=timeout))
    status(out, 'running', active=active)
    supervisor = subprocess.Popen([sys.executable, str(Path(__file__).resolve()), '--stage', 'call', '--out', str(out),
                                   '--command', json.dumps(command), '--result-file', str(result_file),
                                   '--timeout', str(timeout), '--kind', kind], stdin=subprocess.DEVNULL, start_new_session=True)
    # supervisor 有自己的10秒计时；不把外层Python启动计入原生solver耗时。
    supervisor.wait()
    if not result_file.exists():
        result = {'status':'supervisor_error', 'returncode':supervisor.returncode, 'stdout':'', 'stderr':'supervisor did not save a result', 'wall_seconds':None}
    else:
        result = json.loads(result_file.read_text())
    result = classify_phase(phase, result)
    event = dict(type='result', path=case['path'], phase=phase, attempt=attempt, **result)
    append_event(out, event)
    if (phase in ('z3', 'clpoly') and result['status'] in ('error', 'supervisor_error')) or (phase == 'verify' and result['status'] == 'different'):
        append_event(out, dict(type='alert', path=case['path'], phase=phase, kind=result['status'], attempt=attempt))
    print(index+1, phase, result['status'], result.get('wall_seconds'), case['path'], flush=True)
    return event


def skipped(out, case, phase, reason):
    event = dict(type='result', path=case['path'], phase=phase, status='not_run', reason=reason, wall_seconds=None)
    append_event(out,event)
    return event


def worker(out):
    out = out.resolve()
    with (out/'worker.lock').open('a') as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX|fcntl.LOCK_NB)
        except BlockingIOError:
            raise ValueError('another worker already owns this run')
        signal.signal(signal.SIGINT, request_stop)
        signal.signal(signal.SIGTERM, request_stop)
        environment = json.loads((out/'environment.json').read_text())
        manifest = json.loads((out/'manifest.json').read_text())
        check_hashes(environment)
        records = read_events(out, repair=True)
        results = completed(records)
        finished_attempts = {e.get('attempt') for e in records if e.get('type') == 'result'}
        # 恢复 worker 崩溃时仍由独立 supervisor 负责的调用。
        for event in records:
            if event.get('type') != 'start' or event['attempt'] in finished_attempts:
                continue
            result_file = Path(event['result_file'])
            deadline = time.monotonic()+15
            while not result_file.exists() and time.monotonic()<deadline:
                time.sleep(.1)
            result = json.loads(result_file.read_text()) if result_file.exists() else {'status':'interrupted','wall_seconds':None}
            result = classify_phase(event['phase'], result)
            recovered = dict(type='result', path=event['path'], phase=event['phase'], attempt=event['attempt'], recovered=True, **result)
            append_event(out,recovered)
            if recovered['status'] != 'interrupted': results[phase_key(recovered)] = recovered
        frozen = out/'frozen'
        binary, z3 = Path(environment['binary']), Path(environment['z3'])
        for index, case in enumerate(manifest['cases']):
            source = out/'inputs'/case['path']
            txt = out/'converted'/(case['path']+'.txt')
            mapping = out/'mappings'/(case['path']+'.json')
            if all((case['path'],p) in results for p in ('convert','verify','z3','clpoly')):
                continue
            if benchmark.digest(source) != case['sha256']:
                raise ValueError('source hash changed: '+case['path'])
            phases = ['convert','verify'] + (['z3','clpoly'] if index%2==0 else ['clpoly','z3'])
            for phase in phases:
                key = case['path'],phase
                if key in results: continue
                if not await_permission(out):
                    status(out,'stopped'); summarize(out); return
                if phase == 'convert':
                    command = [sys.executable, str(frozen/'full_benchmark.py'), '--stage','convert-one','--out',str(out),
                               '--source',str(source),'--output',str(txt),'--mapping',str(mapping),'--binary',str(binary)]
                    result = execute_phase(out,case,phase,command,environment['conversion_timeout_seconds'],'json',index)
                elif phase == 'verify':
                    if results[(case['path'],'convert')]['status'] != 'converted':
                        result = skipped(out,case,phase,'conversion did not complete')
                    else:
                        command = [sys.executable,str(frozen/'verify.py'),'--source',str(source),'--converted',str(txt)]
                        result = execute_phase(out,case,phase,command,environment['verification_timeout_seconds'],'json',index)
                elif phase == 'clpoly':
                    if results[(case['path'],'convert')]['status'] != 'converted':
                        result = skipped(out,case,phase,'conversion did not complete; no CAD invocation')
                    elif results[(case['path'],'verify')]['status'] != 'equivalent':
                        result = skipped(out,case,phase,'conversion equivalence not established; no CAD invocation')
                    else:
                        if benchmark.digest(txt) != results[(case['path'],'convert')]['details']['txt_sha256']:
                            raise ValueError('saved TXT hash changed')
                        result = execute_phase(out,case,phase,[str(binary),str(txt)],10,'native',index)
                else:
                    result = execute_phase(out,case,phase,[str(z3),'-smt2',str(source)],10,'native',index)
                results[key] = result
            if (index+1)%10==0:
                check_hashes(environment)
                summarize(out)
            if (case['path'],'z3') in results and (case['path'],'clpoly') in results:
                a,b=results[(case['path'],'z3')],results[(case['path'],'clpoly')]
                if a['status'] in ('sat','unsat') and b['status'] in ('sat','unsat') and a['status']!=b['status']:
                    append_event(out,dict(type='alert',path=case['path'],kind='answer_conflict'))
        check_hashes(environment)
        summarize(out)
        status(out,'complete')


def start(out, paused):
    (out/'control').mkdir(exist_ok=True)
    if paused: (out/'control/pause').write_text('pause requested before start\n')
    if (out/'control/stop').exists(): (out/'control/stop').unlink()
    command=[sys.executable,str(out/'frozen/full_benchmark.py'),'--stage','worker','--out',str(out)]
    with (out/'run.log').open('a') as log:
        process=subprocess.Popen(command,stdin=subprocess.DEVNULL,stdout=log,stderr=subprocess.STDOUT,start_new_session=True,close_fds=True)
    atomic_json(out/'launcher.json',dict(pid=process.pid,started_at=utc(),command=command))
    time.sleep(.2)
    if process.poll() is not None: raise ValueError('worker exited immediately; inspect run.log')
    print(json.dumps({'pid':process.pid,'paused':paused,'out':str(out)}))


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--stage',required=True,choices=['prepare','start','worker','convert-one','call','pause','resume','stop','status'])
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--archive',type=Path)
    p.add_argument('--binary',type=Path,default=ROOT/'_build/release/bin/clpoly-cad')
    p.add_argument('--z3',type=Path,default=Path('/opt/homebrew/bin/z3'))
    p.add_argument('--seed',default='20261106-full')
    p.add_argument('--paused',action='store_true')
    p.add_argument('--source',type=Path);p.add_argument('--output',type=Path);p.add_argument('--mapping',type=Path)
    p.add_argument('--command');p.add_argument('--result-file',type=Path);p.add_argument('--timeout',type=float,default=10)
    p.add_argument('--kind',choices=['native','json'],default='native')
    a=p.parse_args();out=a.out.resolve()
    if a.stage=='prepare':
        if a.archive is None:p.error('--archive required')
        prepare(a.archive.resolve(),out,a.binary.resolve(),a.z3.resolve(),a.seed)
    elif a.stage=='start':start(out,a.paused)
    elif a.stage=='worker':worker(out)
    elif a.stage=='convert-one':print(json.dumps(convert_one(a.source,a.output,a.mapping,a.binary)))
    elif a.stage=='call':call(json.loads(a.command),a.result_file,a.timeout,a.kind)
    elif a.stage in ('pause','resume','stop'):
        (out/'control').mkdir(exist_ok=True)
        if a.stage=='resume':
            if (out/'control/pause').exists():(out/'control/pause').unlink()
        else:(out/('control/'+a.stage)).write_text(utc()+'\n')
    else:
        print(json.dumps({'status':json.loads((out/'status.json').read_text()),'summary':summarize(out)},ensure_ascii=False,indent=2))


if __name__=='__main__':
    main()
