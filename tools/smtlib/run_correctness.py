"""先转换／核对，再让两个原生求解器读取落盘输入的正确性实验。"""
import argparse
import json
import shutil
import subprocess
from pathlib import Path

import benchmark


def run(out, binary, z3, timeout):
    manifest = json.loads((out / 'selection.json').read_text())
    verified = {r['path']: r['status'] for r in json.loads((out / 'equivalence.json').read_text())}
    if any(verified.get(r['path']) != 'equivalent' for r in manifest['selected']):
        raise ValueError('every selected conversion must first pass equivalence verification')
    source_hashes = {str(p): benchmark.digest(p) for p in [binary, z3, Path(__file__)]}
    benchmark.dump(out / 'environment.json', {
        'production_commit': subprocess.check_output(['git', '-C', str(benchmark.ROOT), 'rev-parse', 'HEAD'], text=True).strip(),
        'timeout_seconds': timeout, 'sha256': source_hashes,
        'z3': subprocess.check_output([str(z3), '--version'], text=True).strip(),
        'timing': 'Parser conversion and XOR verification finish before timing. Independent serial native solvers, alternating per case. CAD reads renamed TXT; Z3 reads original SMT2; each call has a 10s wall limit.',
    })
    results = []
    for index, case in enumerate(manifest['selected']):
        source = out / 'inputs' / case['path']
        txt = out / 'converted' / (case['path'] + '.txt')
        if benchmark.digest(source) != case['sha256'] or benchmark.digest(txt) != case['txt_sha256']:
            raise ValueError('source or converted input changed')
        commands = {'z3': [str(z3), '-smt2', str(source)], 'clpoly': [str(binary), str(txt)]}
        row = {'path': case['path'], 'family': Path(case['path']).parts[2],
               'declared_status': case['expected'], 'variables': case['variables']}
        for solver in (['z3', 'clpoly'] if index % 2 == 0 else ['clpoly', 'z3']):
            call = benchmark.run_process(commands[solver], timeout)
            call['matches_declared'] = call['status'] == case['expected'] if call['status'] in ('sat', 'unsat') else None
            row[solver] = call
            print(index+1, solver, call['status'], f"{call['wall_seconds']:.4f}", case['path'], flush=True)
        a, b = row['z3']['status'], row['clpoly']['status']
        row['agreement'] = a == b if a in ('sat', 'unsat') and b in ('sat', 'unsat') else None
        results.append(row)
        benchmark.dump(out / 'results.json', results)
    if any(benchmark.digest(p) != sha for p, sha in source_hashes.items()):
        raise ValueError('solver or runner changed during execution')
    benchmark.summarize(out)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--binary', type=Path, default=benchmark.ROOT / '_build/release/bin/clpoly-cad')
    parser.add_argument('--z3', type=Path, default=Path(shutil.which('z3') or 'z3'))
    parser.add_argument('--timeout', type=float, default=10)
    args = parser.parse_args()
    run(args.out, args.binary.resolve(), args.z3.resolve(), args.timeout)
