"""独立核对 CAD --sample 的原坐标证书：非空，并蕴含原 SMT 公式。"""
import argparse
import json
import re
import sys
from fractions import Fraction
from pathlib import Path

import benchmark
import verify
import z3


def certificate(text, aliases):
    lines = text.splitlines()
    if not lines or lines[0] != 'SAT': raise ValueError('not a SAT certificate')
    names = lines[1].removeprefix('sample_variables: ').split()
    if set(names) != set(aliases.values()) or len(names) != len(aliases):
        raise ValueError('sample variable set mismatch')
    variables = {alias: z3.Real(original) for original, alias in aliases.items()}
    equations = lines.index('equations:')
    intervals = lines.index('isolating_intervals:')
    if intervals-equations-1 != len(names): raise ValueError('wrong number of equations')
    constraints = []
    for line in lines[equations+1:intervals]:
        if not line.endswith(' = 0'): raise ValueError('invalid sample equation')
        constraints.append(verify.infix_z3(line[:-4], variables) == 0)
    seen = []
    for line in lines[intervals+1:]:
        if line.startswith('shifts:'): break
        match = re.fullmatch(r'(\S+) in \(([^,]+), ([^)]+)\)', line)
        if not match: raise ValueError('invalid sample interval')
        name, left, right = match.groups()
        if name not in variables or Fraction(left) >= Fraction(right): raise ValueError('invalid sample bounds')
        constraints.extend([variables[name] > z3.RealVal(left), variables[name] < z3.RealVal(right)])
        seen.append(name)
    if seen != names: raise ValueError('sample interval order mismatch')
    return z3.And(*constraints)


def check(source, sample, mapping):
    metadata = json.loads(mapping.read_text())
    aliases = metadata['variable_names'] if 'variable_names' in metadata else metadata['aliases']
    constraints = certificate(sample.read_text(), aliases)
    original = z3.And(*z3.parse_smt2_file(str(source)))
    existence = z3.SolverFor('QF_NRA'); existence.set(timeout=10000)
    existence.add(constraints)
    a = existence.check()
    implication = z3.SolverFor('QF_NRA'); implication.set(timeout=10000)
    implication.add(constraints, z3.Not(original))
    b = implication.check()
    return {'status': 'valid' if a == z3.sat and b == z3.unsat else 'invalid' if a == z3.unsat or b == z3.sat else 'unknown',
            'certificate_nonempty': str(a), 'certificate_and_not_original': str(b)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=Path)
    parser.add_argument('--source', type=Path)
    parser.add_argument('--sample', type=Path)
    parser.add_argument('--mapping', type=Path)
    parser.add_argument('--limit', type=int, default=5)
    args = parser.parse_args()
    if args.source:
        try:
            result = check(args.source, args.sample, args.mapping)
        except Exception as error:
            result = {'status': 'error', 'error': str(error)}
        print(json.dumps(result))
        return
    if args.out is None: parser.error('--out required')
    cases = [r for r in json.loads((args.out/'results.json').read_text()) if r['clpoly']['status']=='sat'][:args.limit]
    environment = json.loads((args.out/'environment.json').read_text())
    binary = next(Path(p) for p in environment['sha256'] if Path(p).name == 'clpoly-cad')
    results = []
    for case in cases:
        txt = args.out/'converted'/(case['path']+'.txt')
        call = benchmark.run_process([str(binary), '--sample', str(txt)],10)
        row = {'path':case['path'],'sample_status':call['status'],'sample_seconds':call['wall_seconds']}
        if call['status'] == 'sat':
            sample = args.out/'witnesses'/(case['path']+'.txt')
            sample.parent.mkdir(parents=True,exist_ok=True);sample.write_text(call['stdout'])
            checked = benchmark.run_process([
                sys.executable,str(Path(__file__).resolve()),'--source',str(args.out/'inputs'/case['path']),
                '--sample',str(sample),'--mapping',str(args.out/'mappings'/(case['path']+'.json'))],10)
            if checked['status']=='timeout':row['verification']={'status':'timeout'}
            else:
                try: row['verification']=json.loads(checked['stdout'])
                except ValueError: row['verification']={'status':'error','stderr':checked['stderr']}
        results.append(row)
        benchmark.dump(args.out/'witnesses.json',results)
        print(json.dumps(row),flush=True)


if __name__=='__main__':main()
