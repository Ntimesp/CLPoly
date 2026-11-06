"""独立检查题集提供的 SAT 见证：有理系数的多二次根式精确运算。

不调用 CLPoly 或 Z3。字典 r:c 表示 c*sqrt(r) 的和，r 为正平方自由整数。
乘法用 gcd 合并相同平方因子；符号用有理平方根包络判断。
"""
import argparse
import ast
import json
from fractions import Fraction as Q
from math import gcd, isqrt
from pathlib import Path


def add(a, b):
    result = a.copy()
    for r, c in b.items():
        result[r] = result.get(r, Q(0)) + c
    return {r: c for r, c in result.items() if c}


def multiply(a, b):
    result = {}
    for r, c in a.items():
        for s, d in b.items():
            g = gcd(r, s)
            root = r * s // (g * g)
            result[root] = result.get(root, Q(0)) + c * d * g
    return {r: c for r, c in result.items() if c}


def power(value, n):
    result = {1: Q(1)}
    for _ in range(n):
        result = multiply(result, value)
    return result


def parse(text):
    def visit(node):
        if isinstance(node, ast.Constant) and type(node.value) is int:
            return {1: Q(node.value)} if node.value else {}
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.UAdd, ast.USub)):
            sign = -1 if isinstance(node.op, ast.USub) else 1
            return {r: sign * c for r, c in visit(node.operand).items()}
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and node.func.id == 'sqrt':
            if len(node.args) != 1 or node.keywords or not isinstance(node.args[0], ast.Constant):
                raise ValueError('sqrt requires one positive integer')
            n = node.args[0].value
            if type(n) is not int or n <= 0:
                raise ValueError('sqrt requires one positive integer')
            scale = 1
            for d in range(2, isqrt(n) + 1):
                while n % (d * d) == 0:
                    n //= d * d
                    scale *= d
            return {n: Q(scale)}
        if isinstance(node, ast.BinOp):
            a, b = visit(node.left), visit(node.right)
            if isinstance(node.op, ast.Add):
                return add(a, b)
            if isinstance(node.op, ast.Sub):
                return add(a, {r: -c for r, c in b.items()})
            if isinstance(node.op, ast.Mult):
                return multiply(a, b)
            if isinstance(node.op, ast.Div):
                if set(b) != {1} or not b[1]:
                    raise ValueError('division requires a nonzero rational')
                return {r: c / b[1] for r, c in a.items()}
        raise ValueError(text)
    return visit(ast.parse(str(text), mode='eval').body)


def sign(value):
    if not value:
        return 0
    bits = 8
    while bits <= 1024:
        denominator = 1 << bits
        lower = upper = Q(0)
        for root, coefficient in value.items():
            integer = isqrt(root * denominator * denominator)
            a = Q(integer, denominator)
            b = a if integer * integer == root * denominator * denominator else a + Q(1, denominator)
            if coefficient < 0:
                a, b = b, a
            lower += coefficient * a
            upper += coefficient * b
        if lower > 0:
            return 1
        if upper < 0:
            return -1
        bits *= 2
    raise ValueError('unresolved nonzero radical sign')


def main(out):
    sources = json.loads((out / 'sources.json').read_text())
    cases = {r['id']: r for r in json.loads((out / 'cases.json').read_text())}
    results = []
    for dataset in ['FormalMath500', 'AIME-2024', 'AIME-2025']:
        count = 0
        for row in [s for s in sources if s['dataset'] == dataset]:
            for query in row['model']['queries']:
                count += 1
                if query['expected_status'] != 'sat':
                    continue
                ident = f'{dataset}-{count:03}'
                case = cases[ident]
                witness = query['witness']
                if isinstance(witness, dict):
                    values = [parse(witness[v]) for v in query.get('variables', row['model']['variables'])]
                else:
                    values = [parse(v) for v in witness]
                if len(values) != len(case['variables']):
                    raise ValueError(f'witness dimension mismatch: {ident}')
                signs = []
                for polynomial in case['polynomials']:
                    value = {}
                    for term in polynomial:
                        monomial = {1: Q(term['num'], term['den'])}
                        for v, e in zip(values, term['exp']):
                            monomial = multiply(monomial, power(v, e))
                        value = add(value, monomial)
                    signs.append(sign(value))
                valid = all(any({'=': signs[i] == 0, '>': signs[i] > 0, '<': signs[i] < 0}[op]
                                for i, op in clause) for clause in case['clauses'])
                if not valid:
                    raise ValueError(f'invalid SAT witness: {ident}')
                results.append({'id': ident, 'witness': witness, 'signs': signs, 'valid': valid})
    if len(results) != 25:
        raise ValueError('expected 25 SAT witnesses')
    (out / 'exact-witnesses.json').write_text(json.dumps(results, ensure_ascii=False, indent=2) + '\n')
    print('25 SAT witnesses checked exactly without CLPoly or Z3')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=Path, required=True)
    main(parser.parse_args().out)
