"""官方 SMT-LIB QF_NRA 抽样及独立进程、严格墙钟限时的双求解器比较。"""
import argparse
import csv
import hashlib
import json
import os
import platform
import re
import signal
import subprocess
import sys
import tarfile
import time
from collections import Counter, defaultdict
from contextlib import contextmanager
from pathlib import Path

URL = 'https://zenodo.org/records/16740866/files/QF_NRA.tar.zst?download=1'
MD5 = '85a04fcea6dd10fb4c505703cac46e2c'
ROOT = Path(__file__).resolve().parents[2]
ADAPTER = Path(__file__).with_name('adapter.py')


def digest(path, algorithm='sha256'):
    result = hashlib.new(algorithm)
    with Path(path).open('rb') as source:
        while block := source.read(1048576):
            result.update(block)
    return result.hexdigest()


def dump(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n')


@contextmanager
def archive_stream(path):
    process = subprocess.Popen(['zstd', '-dc', str(path)], stdout=subprocess.PIPE)
    try:
        with tarfile.open(fileobj=process.stdout, mode='r|') as archive:
            yield archive
        process.stdout.close()
        if process.wait() != 0:
            raise ValueError('zstd decompression failed')
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait()


def select(archive_path, out, count, seed):
    if count <= 0:
        raise ValueError('count must be positive')
    if digest(archive_path, 'md5') != MD5:
        raise ValueError('official archive MD5 mismatch')
    families = defaultdict(list)
    with archive_stream(archive_path) as archive:
        for member in archive:
            if member.isfile() and member.name.endswith('.smt2'):
                parts = Path(member.name).parts
                if parts[:2] != ('non-incremental', 'QF_NRA') or '..' in parts:
                    raise ValueError('unexpected archive member: ' + member.name)
                families[parts[2]].append(dict(path=member.name, bytes=member.size))
    for files in families.values():
        files.sort(key=lambda r: hashlib.sha256((seed + ':' + r['path']).encode()).hexdigest())
    selected = []
    for i in range(max(map(len, families.values()))):
        for name in sorted(families):
            if i < len(families[name]):
                selected.append(families[name][i])
            if len(selected) == count:
                break
        if len(selected) == count:
            break
    wanted = {r['path']: r for r in selected}
    with archive_stream(archive_path) as archive:
        for member in archive:
            if member.name in wanted:
                data = archive.extractfile(member).read()
                path = out / 'inputs' / member.name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
                wanted[member.name]['sha256'] = hashlib.sha256(data).hexdigest()
    dump(out / 'selection.json', {
        'release': '2025.08.04', 'url': 'https://zenodo.org/records/16740866',
        'logic': 'QF_NRA', 'seed': seed,
        'rule': 'Round robin sorted families; SHA256(seed + colon + original path) within each family; no status or solver filtering.',
        'archive': {'url': URL, 'bytes': archive_path.stat().st_size, 'md5': MD5, 'sha256': digest(archive_path)},
        'population': {k: len(v) for k, v in sorted(families.items())}, 'selected': selected,
    })
    print('selected', len(selected), 'from', sum(map(len, families.values())), flush=True)


def run_process(command, timeout):
    start = time.monotonic()
    process = subprocess.Popen(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               start_new_session=True)
    timed_out = False
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        timed_out = True
        # 适配器的 CAD 子进程继承同一进程组，超时必须一起回收。
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        stdout, stderr = process.communicate()
    elapsed = time.monotonic() - start
    answers = [s.strip().lower() for s in stdout.splitlines()
               if s.strip().lower() in ('sat', 'unsat', 'unknown')]
    if timed_out:
        status = 'timeout'
    elif process.returncode == 3 and stderr.startswith('unsupported:'):
        status = 'unsupported'
    elif process.returncode != 0 or '(error' in stdout or len(answers) != 1:
        status = 'error'
    else:
        status = answers[0]
    result = dict(command=command, status=status, returncode=process.returncode,
                  wall_seconds=elapsed, stdout=stdout, stderr=stderr)
    if timed_out:
        result['timeout_seconds'] = timeout
    for line in stderr.splitlines():
        if line.startswith('{'):
            try:
                result['conversion'] = json.loads(line)
            except ValueError:
                pass
    return result


def run(out, binary, z3, timeout):
    if timeout <= 0:
        raise ValueError('timeout must be positive')
    manifest = json.loads((out / 'selection.json').read_text())
    for case in manifest['selected']:
        if digest(out / 'inputs' / case['path']) != case['sha256']:
            raise ValueError('input hash mismatch: ' + case['path'])
    hashes = {str(p): digest(p) for p in [binary, z3, ADAPTER, Path(__file__)]}
    dump(out / 'environment.json', {
        'platform': platform.platform(), 'python': sys.version, 'timeout_seconds': timeout,
        'z3': subprocess.check_output([str(z3), '--version'], text=True).strip(),
        'production_commit': subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip(),
        'sha256': hashes,
        'timing': 'Separate serial processes; order alternates per case. CLPoly time includes Python SMT conversion, process startup, TXT parse and full CAD. Z3 reads original SMT-LIB. No original/reverse retry or linear preprocessing.',
    })
    rows = []
    for i, case in enumerate(manifest['selected']):
        path = out / 'inputs' / case['path']
        row = dict(path=case['path'], family=Path(case['path']).parts[2])
        match = re.search(r'\(set-info\s+:status\s+(sat|unsat|unknown)\s*\)', path.read_text())
        row['declared_status'] = match[1] if match else 'unknown'
        converted = out / 'converted' / (case['path'] + '.txt')
        converted.parent.mkdir(parents=True, exist_ok=True)
        commands = {
            'z3': [str(z3), '-smt2', str(path)],
            'clpoly': [sys.executable, str(ADAPTER), str(path), '--binary', str(binary), '--output', str(converted)],
        }
        for solver in (['z3', 'clpoly'] if i % 2 == 0 else ['clpoly', 'z3']):
            row[solver] = run_process(commands[solver], timeout)
            print(i+1, solver, row[solver]['status'], f"{row[solver]['wall_seconds']:.3f}", row['path'], flush=True)
        a, b = row['z3']['status'], row['clpoly']['status']
        row['agreement'] = a == b if a in ('sat', 'unsat') and b in ('sat', 'unsat') else None
        for solver in ('z3', 'clpoly'):
            status = row[solver]['status']
            row[solver]['matches_declared'] = status == row['declared_status'] if status in ('sat', 'unsat') and row['declared_status'] != 'unknown' else None
        rows.append(row)
        dump(out / 'results.json', rows)
    if any(digest(p) != value for p, value in hashes.items()):
        raise ValueError('binary or experiment source changed during run')
    summarize(out)


def summarize(out):
    rows = json.loads((out / 'results.json').read_text())
    result = {'cases': len(rows)}
    result['solvers'] = {solver: dict(Counter(r[solver]['status'] for r in rows)) for solver in ('z3', 'clpoly')}
    both = [r for r in rows if r['agreement'] is not None]
    result['both_decided'] = len(both)
    result['disagreements'] = [r['path'] for r in both if not r['agreement']]
    result['declared_mismatches'] = {solver: [r['path'] for r in rows if r[solver]['matches_declared'] is False]
                                     for solver in ('z3', 'clpoly')}
    result['families'] = {}
    for family in sorted({r['family'] for r in rows}):
        group = [r for r in rows if r['family'] == family]
        result['families'][family] = {solver: dict(Counter(r[solver]['status'] for r in group))
                                       for solver in ('z3', 'clpoly')}
    if both:
        result['common_solved_seconds'] = {solver: sum(r[solver]['wall_seconds'] for r in both) for solver in ('z3', 'clpoly')}
    dump(out / 'summary.json', result)
    with (out / 'results.csv').open('w', newline='') as stream:
        writer = csv.writer(stream)
        writer.writerow(['path', 'family', 'declared', 'z3', 'z3_seconds', 'clpoly', 'clpoly_seconds', 'agreement'])
        for row in rows:
            writer.writerow([row['path'], row['family'], row['declared_status'], row['z3']['status'], row['z3']['wall_seconds'],
                             row['clpoly']['status'], row['clpoly']['wall_seconds'], row['agreement']])
    print(json.dumps(result, indent=2), flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--stage', choices=['select', 'run', 'summarize'], required=True)
    parser.add_argument('--archive', type=Path)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--count', type=int, default=100)
    parser.add_argument('--seed', default='20261106')
    parser.add_argument('--timeout', type=float, default=10)
    parser.add_argument('--binary', type=Path, default=ROOT / '_build/release/bin/clpoly-cad')
    parser.add_argument('--z3', type=Path, default=Path('/opt/homebrew/bin/z3'))
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    if args.stage == 'select':
        if args.archive is None: parser.error('--archive is required for selection')
        select(args.archive, args.out, args.count, args.seed)
    elif args.stage == 'run':
        run(args.out, args.binary.resolve(), args.z3.resolve(), args.timeout)
    else:
        summarize(args.out)


if __name__ == '__main__':
    main()
