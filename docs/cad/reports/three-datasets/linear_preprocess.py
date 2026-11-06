"""实验用精确仿射消元；不属于库的公共接口。

仅采用单原子子句中的总次数至多 1 的等式，保留所有恢复表达式。
规格见 docs/cad/design/formula-preprocessing.md。
"""
from fractions import Fraction as Q
import time

def add(a, b):
    c = a.copy()
    for e, v in b.items():
        c[e] = c.get(e, Q(0)) + v
    return {e: v for e, v in c.items() if v}

def mul(a, b):
    c = {}
    for e, v in a.items():
        for f, w in b.items():
            k = tuple((x + y for x, y in zip(e, f)))
            c[k] = c.get(k, Q(0)) + v * w
    return {e: v for e, v in c.items() if v}

def constant(n, c):
    return {(0,) * n: Q(c)} if c else {}

def variable(n, i):
    e = [0] * n
    e[i] = 1
    return {tuple(e): Q(1)}

def substitute(p, index, replacement, n):
    powers = {0: constant(n, 1), 1: replacement}

    def power(k):
        if k not in powers:
            half = power(k // 2)
            powers[k] = mul(half, half)
            if k % 2:
                powers[k] = mul(powers[k], replacement)
        return powers[k]
    result = {}
    for e, c in p.items():
        rest = list(e)
        rest[index] = 0
        result = add(result, mul({tuple(rest): c}, power(e[index])))
    return result

def simplify(polys, clauses, n):
    zero = (0,) * n
    result = []
    for clause in clauses:
        atoms = []
        truth = False
        for i, op in clause:
            p = polys[i]
            if all((e == zero for e in p)):
                v = p.get(zero, Q(0))
                yes = {'=': v == 0, '>': v > 0, '<': v < 0}[op]
                if yes:
                    truth = True
                    break
            elif (i, op) not in atoms:
                atoms.append((i, op))
        if truth:
            continue
        if not atoms:
            return ([[]], 'unsat')
        result.append(atoms)
    return (result, 'sat' if not result else 'residual')

def validate_case(case):
    variables = case['variables']
    if (any(not isinstance(v, str) or not v or any(c.isspace() for c in v) for v in variables)
            or len(set(variables)) != len(variables)):
        raise ValueError('invalid variable list')
    n = len(variables)
    for polynomial in case['polynomials']:
        for term in polynomial:
            if (len(term['exp']) != n or any(type(e) is not int or e < 0 for e in term['exp'])
                    or type(term['num']) is not int or type(term['den']) is not int or term['den'] == 0):
                raise ValueError('invalid rational polynomial term')
    for clause in case['clauses']:
        for atom in clause:
            if (len(atom) != 2 or type(atom[0]) is not int
                    or not 0 <= atom[0] < len(case['polynomials']) or atom[1] not in ('=', '>', '<')):
                raise ValueError('invalid atom reference or relation')


def preprocess(case):
    validate_case(case)
    n = len(case['variables'])
    polys = []
    for polynomial in case['polynomials']:
        p = {}
        for term in polynomial:
            p = add(p, {tuple(term['exp']): Q(term['num'], term['den'])})
        polys.append(p)
    clauses = [[tuple(a) for a in c] for c in case['clauses']]
    live = list(range(n))
    maps = [variable(n, i) for i in live]
    steps = []
    start = time.monotonic()
    while True:
        clauses, status = simplify(polys, clauses, n)
        if status != 'residual':
            break
        candidates = []
        for clause in clauses:
            if len(clause) != 1 or clause[0][1] != '=':
                continue
            i = clause[0][0]
            p = polys[i]
            if any((sum(e) > 1 for e in p)):
                continue
            variables = [j for j in live if p.get(tuple((1 if k == j else 0 for k in range(n))), Q(0))]
            if not variables:
                continue
            pivot = max(variables)
            candidates.append((len(variables), len(p), -pivot, i, pivot))
        if not candidates:
            break
        _, _, _, i, pivot = min(candidates)
        key = tuple((1 if k == pivot else 0 for k in range(n)))
        coefficient = polys[i][key]
        rest = polys[i].copy()
        del rest[key]
        replacement = {e: -c / coefficient for e, c in rest.items()}
        steps.append({'equation_index': i, 'pivot': case['variables'][pivot], 'replacement': [{'exp': list(e), 'num': c.numerator, 'den': c.denominator} for e, c in replacement.items()]})
        polys = [substitute(p, pivot, replacement, n) for p in polys]
        maps = [substitute(p, pivot, replacement, n) for p in maps]
        live.remove(pivot)
    used = sorted({i for c in clauses for i, op in c})
    index = {old: new for new, old in enumerate(used)}
    reduced = []
    for i in used:
        reduced.append([{'exp': [e[k] for k in live], 'num': v.numerator, 'den': v.denominator} for e, v in polys[i].items()])
    result = {
        'id': case['id'],
        'variables': [case['variables'][i] for i in live],
        'polynomials': reduced,
        'clauses': [[(index[i], op) for i, op in c] for c in clauses],
        'preprocess_status': status,
        'original_variables': case['variables'],
        'original_dimension': n,
        'remaining_dimension': len(live),
        'steps': steps,
        'reconstruction': [
            [{'exp': [e[k] for k in live], 'num': v.numerator, 'den': v.denominator}
             for e, v in p.items()] for p in maps],
        'preprocess_seconds': time.monotonic() - start,
    }
    if 'z3_status' in case:
        result['z3_status'] = case['z3_status']
    return result
