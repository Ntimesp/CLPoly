"""三个题集的格式、模型一致性、独立对照与 CAD 实验。所有系数精确解析。"""
import argparse
import ast
import csv
import hashlib
import json
import platform
import re
import subprocess
import time
from collections import Counter
from fractions import Fraction as Q
from pathlib import Path

import z3
from linear_preprocess import preprocess, validate_case

REPO = Path(__file__).resolve().parents[4]
DATASETS = ['FormalMath500', 'AIME-2024', 'AIME-2025']


def add(a, b):
    result = a.copy()
    for exponent, coefficient in b.items():
        result[exponent] = result.get(exponent, Q(0)) + coefficient
    return {e: c for e, c in result.items() if c}


def multiply(a, b):
    result = {}
    for e, c in a.items():
        for f, d in b.items():
            powers = tuple(x + y for x, y in zip(e, f))
            result[powers] = result.get(powers, Q(0)) + c * d
    return {e: c for e, c in result.items() if c}


def parse_polynomial(text, variables):
    zero = (0,) * len(variables)

    def visit(node):
        if isinstance(node, ast.Constant) and type(node.value) is int:
            return {zero: Q(node.value)} if node.value else {}
        if isinstance(node, ast.Name) and node.id in variables:
            e = list(zero)
            e[variables.index(node.id)] = 1
            return {tuple(e): Q(1)}
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.UAdd, ast.USub)):
            sign = -1 if isinstance(node.op, ast.USub) else 1
            return {e: sign * c for e, c in visit(node.operand).items()}
        if isinstance(node, ast.BinOp):
            a = visit(node.left)
            if isinstance(node.op, ast.Pow):
                if (not isinstance(node.right, ast.Constant) or type(node.right.value) is not int
                        or not 0 <= node.right.value <= 20):
                    raise ValueError('power must be an integer from 0 to 20')
                result = {zero: Q(1)}
                for _ in range(node.right.value):
                    result = multiply(result, a)
                return result
            b = visit(node.right)
            if isinstance(node.op, ast.Add):
                return add(a, b)
            if isinstance(node.op, ast.Sub):
                return add(a, {e: -c for e, c in b.items()})
            if isinstance(node.op, ast.Mult):
                return multiply(a, b)
            if isinstance(node.op, ast.Div) and len(b) == 1 and b.get(zero):
                return {e: c / b[zero] for e, c in a.items()}
        raise ValueError(f'Unsupported polynomial syntax: {text}')

    return visit(ast.parse(text.replace('^', '**'), mode='eval').body)


def terms(polynomial):
    return [{'exp': list(e), 'num': c.numerator, 'den': c.denominator}
            for e, c in sorted(polynomial.items())]


def polynomial_z3(polynomial, variables):
    # 模型 JSON 与 TXT 项表分别进入这里，不能让 zip 静默截断错误维数。
    for term in polynomial:
        if (len(term['exp']) != len(variables)
                or any(type(e) is not int or e < 0 for e in term['exp'])
                or type(term['num']) is not int or type(term['den']) is not int or not term['den']):
            raise ValueError('invalid polynomial term for reference conversion')
    value = z3.RealVal(0)
    for term in polynomial:
        monomial = z3.RealVal(f"{term['num']}/{term['den']}")
        for variable, exponent in zip(variables, term['exp']):
            if exponent:
                monomial = monomial * variable ** exponent
        value = value + monomial
    return value


def relation(value, operator):
    return {'=': value == 0, '!=': value != 0, '>': value > 0,
            '<': value < 0, '>=': value >= 0, '<=': value <= 0}[operator]


def formula(case, variables):
    polynomials = [polynomial_z3(p, variables) for p in case['polynomials']]
    return z3.And(*[z3.Or(*[relation(polynomials[i], op) for i, op in clause])
                    for clause in case['clauses']])


def solve(expression):
    solver = z3.SolverFor('QF_NRA')
    solver.set(timeout=15000)
    solver.add(expression)
    start = time.monotonic()
    answer = solver.check()
    result = {'status': str(answer), 'seconds': time.monotonic() - start}
    if answer == z3.sat:
        result['model'] = str(solver.model())
    if answer == z3.unknown:
        result['reason'] = solver.reason_unknown()
    return result


def dump(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n')


def load_jsonl(path):
    return [json.loads(line) for line in path.read_text().splitlines() if line.strip()]


def write_input(path, case):
    validate_case(case)
    lines = [f"{len(case['variables'])} {len(case['polynomials'])} {len(case['clauses'])}",
             ' '.join(case['variables'])]
    for polynomial in case['polynomials']:
        lines.append(str(len(polynomial)))
        for term in polynomial:
            lines.append(' '.join(map(str, [term['num'], term['den'], *term['exp']])))
    for clause in case['clauses']:
        lines.append(' '.join([str(len(clause)), *[f'{i} {op}' for i, op in clause]]))
    path.write_text('\n'.join(lines) + '\n')


def prepare(data, out):
    original_math = {r['unique_id']: r for r in load_jsonl(data / '原始数据/MATH-500/test.jsonl')}
    formal_math = load_jsonl(data / '形式化数据/FormalMath500/formal_math500.jsonl')
    cases, audits, sources, reduced = [], [], [], []
    (out / 'inputs').mkdir(exist_ok=True)
    for dataset in DATASETS:
        directory = data / '转换结果' / dataset
        models = load_jsonl(next(directory.glob('*systems.jsonl')))
        original_aime = None
        if dataset != 'FormalMath500':
            original_aime = load_jsonl(next((data / '原始数据' / dataset).glob('*.jsonl')))
        count = 0
        for model in models:
            if dataset == 'FormalMath500':
                source = original_math[model['source_unique_id']]
                formal = formal_math[int(model['id'].split(':')[1]) - 1]
            else:
                source = original_aime[model['source_index'] - 1]
                if str(source['id']) != str(model['source_id']):
                    raise ValueError('source id mismatch')
                formal = model.get('formal_reference')
            sources.append({'dataset': dataset, 'model': model, 'original': source, 'formal': formal})
            for query in model['queries']:
                count += 1
                path = directory / query.get('file', f'FormalMath500_多项式系统子句文件/{count:03}.txt')
                blocks = re.split(r'\n\s*\n', path.read_text().strip())
                if len(blocks) != 3:
                    raise ValueError(f'invalid input blocks: {path}')
                variables = blocks[0].split()
                if len(variables) != len(set(variables)):
                    raise ValueError(f'duplicate variable: {path}')
                polynomials = [terms(parse_polynomial(line, variables)) for line in blocks[1].splitlines()]
                clauses = []
                for line in blocks[2].splitlines():
                    clause = []
                    for atom in line.split('|'):
                        match = re.fullmatch(r'\s*\[\s*(\d+)\s*,\s*([=<>])\s*\]\s*', atom)
                        if not match:
                            raise ValueError(f'invalid atom: {path}: {atom}')
                        index = int(match[1])
                        if index >= len(polynomials):
                            raise ValueError(f'atom index out of range: {path}')
                        clause.append([index, match[2]])
                    clauses.append(clause)
                ident = f'{dataset}-{count:03}'
                case = {'id': ident, 'dataset': dataset, 'number': f'{count:03}',
                        'model_id': model['id'], 'variables': variables,
                        'polynomials': polynomials, 'clauses': clauses,
                        'purpose': query['purpose'], 'expected': query['expected_status'],
                        'source_path': str(path.relative_to(data)),
                        'max_degree': max((sum(t['exp']) for p in polynomials for t in p), default=0)}
                x = [z3.Real(f'x{i}') for i in range(len(variables))]
                reference = solve(formula(case, x))
                case['z3_status'] = reference['status']
                model_variables = query.get('variables', model['variables'])
                if len(model_variables) != len(variables):
                    raise ValueError(f'variable alias count mismatch: {ident}')
                constraints = []
                for atom in query['system']:
                    p = [{'exp': t['powers'], 'num': Q(t['coefficient']).numerator,
                          'den': Q(t['coefficient']).denominator} for t in atom['terms']]
                    constraints.append(relation(polynomial_z3(p, x), atom['relation']))
                json_equivalence = solve(z3.Xor(formula(case, x), z3.And(*constraints)))
                result = preprocess(case)
                u = [z3.Real(f'u{i}') for i in range(len(result['variables']))]
                reconstructed = [polynomial_z3(p, u) for p in result['reconstruction']]
                preprocess_equivalence = solve(z3.Xor(formula(case, reconstructed), formula(result, u)))
                audit = {'id': ident, 'reference': reference, 'matches_label': reference['status'] == case['expected'],
                         'txt_json_xor': json_equivalence, 'preprocess_xor': preprocess_equivalence,
                         'variable_aliases': dict(zip(model_variables, variables))}
                if result['preprocess_status'] == 'sat':
                    # 自由参数置零，用恢复映射得到精确 QQ 见证，再检查全部原子。
                    values = []
                    for p in result['reconstruction']:
                        values.append(sum((Q(t['num'], t['den']) for t in p if not any(t['exp'])), Q(0)))
                    witness = {name: str(value) for name, value in zip(variables, values)}
                    exact_values = []
                    for p in polynomials:
                        value = Q(0)
                        for t in p:
                            monomial = Q(t['num'], t['den'])
                            for v, e in zip(values, t['exp']):
                                monomial *= v ** e
                            value += monomial
                        exact_values.append(value)
                    valid = all(any({'=': exact_values[i] == 0, '>': exact_values[i] > 0,
                                     '<': exact_values[i] < 0}[op] for i, op in c) for c in clauses)
                    if not valid:
                        raise ValueError(f'invalid restored SAT witness: {ident}')
                    audit['direct_sat_witness'] = witness
                cases.append(case)
                reduced.append(result)
                audits.append(audit)
                write_input(out / 'inputs' / f'{ident}-raw.cad', case)
                if result['preprocess_status'] == 'residual':
                    write_input(out / 'inputs' / f'{ident}-reduced.cad', result)
                print(ident, 'reference=' + reference['status'], 'txt_json_xor=' + json_equivalence['status'],
                      'preprocess=' + result['preprocess_status'], flush=True)
    if len(cases) != 72 or len(sources) != 47:
        raise ValueError('unexpected dataset size')
    dump(out / 'cases.json', cases)
    dump(out / 'preprocessed.json', reduced)
    dump(out / 'audit.json', audits)
    dump(out / 'sources.json', sources)
    files = [*data.rglob('*'), *REPO.glob('clpoly/**/*.hh'), *REPO.glob('clpoly/*.cc'),
             REPO / 'lib/debug/libclpoly.a']
    manifest = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in files if p.is_file()}
    dump(out / 'manifest.json', {'platform': platform.platform(), 'machine': platform.machine(),
                               'z3': z3.get_version_string(), 'sha256': manifest})


def run_all(out, limit):
    cases = json.loads((out / 'cases.json').read_text())
    reduced = {r['id']: r for r in json.loads((out / 'preprocessed.json').read_text())}
    rows = []
    for mode in ['raw', 'reduced']:
        for case in cases:
            ident = case['id']
            if mode == 'reduced' and reduced[ident]['preprocess_status'] != 'residual':
                rows.append({'id': ident, 'mode': mode, 'order': 'none',
                             'status': reduced[ident]['preprocess_status'], 'kind': 'preprocess_only',
                             'wall_seconds': reduced[ident]['preprocess_seconds']})
                dump(out / 'results.json', rows)
                continue
            for order in ['default', 'reverse']:
                args = [str(out / 'driver'), str(out / 'inputs' / f'{ident}-{mode}.cad')]
                if order == 'reverse':
                    args.append('reverse')
                start = time.monotonic()
                try:
                    p = subprocess.run(args, text=True, capture_output=True, timeout=limit)
                    fields = dict(re.findall(r'(\w+)=([^\s]+)', p.stdout))
                    status = fields.get('status', 'error') if p.returncode == 0 else 'error'
                    row = {'id': ident, 'mode': mode, 'order': order, 'status': status,
                           'returncode': p.returncode, 'stdout': p.stdout, 'stderr': p.stderr,
                           'wall_seconds': time.monotonic() - start, 'fields': fields, 'kind': 'cad'}
                except subprocess.TimeoutExpired:
                    row = {'id': ident, 'mode': mode, 'order': order, 'status': 'timeout',
                           'wall_seconds': time.monotonic() - start, 'timeout_seconds': limit, 'kind': 'cad'}
                rows.append(row)
                dump(out / 'results.json', rows)
                print(ident, mode, order, row['status'],
                      f"{row['wall_seconds']:.3f}s", flush=True)
                if row['status'] in ['sat', 'unsat']:
                    break
    summarize(out)


def summarize(out):
    cases = json.loads((out / 'cases.json').read_text())
    rows = json.loads((out / 'results.json').read_text())
    reduced = {r['id']: r for r in json.loads((out / 'preprocessed.json').read_text())}
    table, counts = [], {}
    for case in cases:
        item = {k: case[k] for k in ['id', 'dataset', 'number', 'model_id', 'purpose', 'expected', 'max_degree']}
        item.update(original_variables=len(case['variables']), remaining_variables=len(reduced[case['id']]['variables']))
        for mode in ['raw', 'reduced']:
            group = [r for r in rows if r['id'] == case['id'] and r['mode'] == mode]
            if not group:
                continue
            result = group[-1]
            item[mode] = result['status']
            item[mode + '_seconds'] = result['wall_seconds']
            item[mode + '_order'] = result['order']
            item[mode + '_kind'] = result['kind']
            item[mode + '_cad_attempts'] = sum(r['kind'] == 'cad' for r in group)
            item[mode + '_total_wall_seconds'] = sum(r['wall_seconds'] for r in group)
            item[mode + '_matches_reference'] = (result['status'] == case['z3_status']
                                                 if result['status'] in ['sat', 'unsat'] else None)
        table.append(item)
    for dataset in DATASETS:
        counts[dataset] = {mode: dict(Counter(r.get(mode, 'pending') for r in table if r['dataset'] == dataset))
                           for mode in ['raw', 'reduced']}
    dump(out / 'summary.json', counts)
    dump(out / 'table.json', table)
    with (out / 'results.csv').open('w') as f:
        writer = csv.DictWriter(f, fieldnames=list(dict.fromkeys(k for row in table for k in row)))
        writer.writeheader()
        writer.writerows(table)
    print(json.dumps(counts, indent=2), flush=True)


def check_premises(out):
    results = []
    for case in json.loads((out / 'cases.json').read_text()):
        if case['expected'] != 'unsat':
            continue
        # 本题集的反例/越界原子统一放在最后一条子句。
        premises = dict(case, clauses=case['clauses'][:-1])
        variables = [z3.Real(f'x{i}') for i in range(len(case['variables']))]
        result = solve(formula(premises, variables))
        results.append({'id': case['id'], **result})
    dump(out / 'premises.json', results)
    print('UNSAT queries, premises:', dict(Counter(r['status'] for r in results)))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--data', type=Path, default=Path('/Users/nxp/Projects/CAD测试题集_三数据集'))
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--stage', choices=['prepare', 'run', 'summarize', 'premises'], required=True)
    parser.add_argument('--timeout', type=float, default=10)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    if args.stage == 'prepare':
        prepare(args.data, args.out)
    elif args.stage == 'run':
        run_all(args.out, args.timeout)
    elif args.stage == 'premises':
        check_premises(args.out)
    else:
        summarize(args.out)


if __name__ == '__main__':
    main()
