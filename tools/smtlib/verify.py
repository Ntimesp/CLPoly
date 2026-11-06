"""计时后核对实际生成的 TXT 与原始 SMT-LIB 等价；不参与 CLPoly 决策。"""
import argparse
import ast
import hashlib
import json
import re
import sys
from pathlib import Path

import adapter
import benchmark
import z3


def infix_z3(text, variables):
    def visit(node):
        if isinstance(node, ast.Constant) and type(node.value) is int:
            return z3.RealVal(node.value)
        if isinstance(node, ast.Name):
            return variables[node.id]
        if isinstance(node, ast.UnaryOp):
            value = visit(node.operand)
            if isinstance(node.op, ast.USub): return -value
            if isinstance(node.op, ast.UAdd): return value
        if isinstance(node, ast.BinOp):
            left, right = visit(node.left), visit(node.right)
            if isinstance(node.op, ast.Add): return left + right
            if isinstance(node.op, ast.Sub): return left - right
            if isinstance(node.op, ast.Mult): return left * right
            if isinstance(node.op, ast.Div): return left / right
            if isinstance(node.op, ast.Pow): return left ** right
        raise ValueError('unexpected infix syntax: ' + ast.dump(node))
    return visit(ast.parse(text.replace('^', '**'), mode='eval').body)


def verify(source, converted):
    original = z3.And(*z3.parse_smt2_file(str(source)))
    regenerated, ctx, _ = adapter.convert(source.read_text())
    text = converted.read_text()
    if text != regenerated:
        raise ValueError('saved TXT differs from frozen adapter output')
    blocks = text.strip().split('\n\n')
    if len(blocks) != 3:
        raise ValueError('invalid saved TXT blocks')
    variables = {alias: z3.Real(name) for name, alias in ctx.aliases.items()}
    if not variables:
        variables['unused0'] = z3.Real('unused0')
    polynomials = [infix_z3(line, variables) for line in blocks[1].splitlines()]
    clauses = []
    for line in blocks[2].splitlines():
        atoms = []
        for atom in line.split('|'):
            match = re.fullmatch(r'\s*\[(\d+),([=<>])\]\s*', atom)
            if not match: raise ValueError('invalid saved atom')
            p = polynomials[int(match[1])]
            atoms.append({'=': p == 0, '>': p > 0, '<': p < 0}[match[2]])
        clauses.append(z3.Or(*atoms))
    solver = z3.SolverFor('QF_NRA')
    solver.set(timeout=10000)
    solver.add(z3.Xor(original, z3.And(*clauses)))
    result = solver.check()
    return {'status': 'equivalent' if result == z3.unsat else 'different' if result == z3.sat else 'unknown',
            'xor': str(result), 'source_sha256': benchmark.digest(source), 'txt_sha256': benchmark.digest(converted)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=Path)
    parser.add_argument('--source', type=Path)
    parser.add_argument('--converted', type=Path)
    args = parser.parse_args()
    if args.source:
        try:
            result = verify(args.source, args.converted)
        except Exception as error:
            result = {'status': 'error', 'error': str(error)}
        print(json.dumps(result))
        return
    if args.out is None: parser.error('requires --out or --source/--converted')
    rows = []
    for case in json.loads((args.out / 'selection.json').read_text())['selected']:
        converted = args.out / 'converted' / (case['path'] + '.txt')
        if not converted.exists():
            continue
        call = benchmark.run_process([sys.executable, str(Path(__file__).resolve()),
                                      '--source', str(args.out / 'inputs' / case['path']),
                                      '--converted', str(converted)], 10)
        if call['status'] == 'timeout':
            result = {'status': 'timeout'}
        else:
            try:
                result = json.loads(call['stdout'])
            except ValueError:
                result = {'status': 'error', 'stdout': call['stdout'], 'stderr': call['stderr']}
        row = {'path': case['path'], 'wall_seconds': call['wall_seconds'], **result}
        rows.append(row)
        benchmark.dump(args.out / 'equivalence.json', rows)
        print(result['status'], case['path'], flush=True)


if __name__ == '__main__':
    main()
