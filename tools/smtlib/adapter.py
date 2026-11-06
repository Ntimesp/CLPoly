"""SMT-LIB QF_NRA 多项式子集到现有 CAD CLI 的独立适配器。

不导入 Z3，不进行代数消元。支持范围与限额见同目录 README.md。
"""
import argparse
import hashlib
import itertools
import json
import re
import subprocess
import sys
import time
from dataclasses import dataclass, field
from fractions import Fraction
from pathlib import Path


class Unsupported(ValueError):
    pass


class String(str):
    """将 SMT-LIB 字符串与符号区分。"""


def sexpressions(text):
    roots, stack = [], []
    i = 0
    while i < len(text):
        c = text[i]
        if c.isspace():
            i += 1
            continue
        if c == ';':
            end = text.find('\n', i)
            i = len(text) if end < 0 else end + 1
            continue
        if c == '(':
            node = []
            (stack[-1] if stack else roots).append(node)
            stack.append(node)
            i += 1
            continue
        if c == ')':
            if not stack:
                raise Unsupported('unmatched closing parenthesis')
            stack.pop()
            i += 1
            continue
        if c in '|"':
            quote = c
            i += 1
            token = ''
            while i < len(text):
                if text[i] == quote:
                    if quote == '"' and i + 1 < len(text) and text[i + 1] == '"':
                        token += '"'
                        i += 2
                        continue
                    break
                token += text[i]
                i += 1
            if i == len(text):
                raise Unsupported('unterminated quoted token')
            i += 1
            token = String(token) if quote == '"' else token
        else:
            end = i
            while i < len(text) and not text[i].isspace() and text[i] not in '();':
                i += 1
            token = text[end:i]
        (stack[-1] if stack else roots).append(token)
    if stack:
        raise Unsupported('unmatched opening parenthesis')
    return roots


@dataclass
class Context:
    names: list = field(default_factory=list)
    aliases: dict = field(default_factory=dict)
    polynomials: list = field(default_factory=list)
    polynomial_indices: dict = field(default_factory=dict)
    expected: str = 'unknown'


MAX_CLAUSES = 4096
MAX_EXPRESSION = 2_000_000


def bounded(text):
    if len(text) > MAX_EXPRESSION:
        raise Unsupported('expanded expression exceeds 2000000 bytes')
    return text


def rational(value):
    return str(value.numerator) if value.denominator == 1 else f'({value.numerator}/{value.denominator})'


def let_environment(node, env):
    if len(node) != 3 or not isinstance(node[1], list):
        raise Unsupported('invalid let')
    result, seen = dict(env), set()
    for binding in node[1]:
        if len(binding) != 2 or not isinstance(binding[0], str) or binding[0] in seen:
            raise Unsupported('invalid let binding')
        seen.add(binding[0])
        # SMT-LIB let 的 RHS 同时在外层作用域解释，不能顺次替换。
        result[binding[0]] = (binding[1], env)
    return result


def arithmetic(node, env, ctx):
    if isinstance(node, String):
        raise Unsupported('string in arithmetic')
    if isinstance(node, str):
        if node in env:
            term, scope = env[node]
            return arithmetic(term, scope, ctx)
        if node in ctx.aliases:
            return ctx.aliases[node], None
        if re.fullmatch(r'[0-9]+(?:\.[0-9]+)?', node):
            q = Fraction(node)
            return rational(q), q
        raise Unsupported('unsupported arithmetic symbol: ' + node)
    if not node or not isinstance(node[0], str):
        raise Unsupported('unsupported arithmetic term')
    op = node[0]
    if op == 'let':
        return arithmetic(node[2], let_environment(node, env), ctx)
    if op == '!':
        return arithmetic(node[1], env, ctx)
    args = [arithmetic(a, env, ctx) for a in node[1:]]
    if op in ('^', '**') and len(args) == 2:
        exponent = args[1][1]
        if exponent is None or exponent.denominator != 1 or not 0 <= exponent <= 2**63 - 1:
            raise Unsupported('non-polynomial exponent')
        return bounded(f'({args[0][0]})^{int(exponent)}'), None
    if op not in ('+', '-', '*', '/') or not args or (op != '-' and len(args) < 2):
        raise Unsupported('unsupported arithmetic operator: ' + op)
    if op == '/' and any(q is None or q == 0 for _, q in args[1:]):
        # 实除法在零分母处是未指定的总函数；不能擅自清分母。
        raise Unsupported('division by a symbolic or zero denominator')
    value = None
    if all(q is not None for _, q in args):
        value = args[0][1]
        if len(args) == 1:
            value = -value
        for _, q in args[1:]:
            if op == '+': value += q
            elif op == '-': value -= q
            elif op == '*': value *= q
            else: value /= q
    if len(args) == 1:
        text = f'(-({args[0][0]}))'
    else:
        text = '(' + op.join('(' + a + ')' for a, _ in args) + ')'
    return bounded(text), value


def conjunction(forms):
    result = []
    for form in forms:
        if any(not clause for clause in form):
            return [[]]
        result.extend(form)
        if len(result) > MAX_CLAUSES:
            raise Unsupported('CNF exceeds 4096 clauses')
    return result


def disjunction(forms):
    result = [[]]
    for form in forms:
        if not form:
            return []
        if len(result) * len(form) > MAX_CLAUSES:
            raise Unsupported('CNF distribution exceeds 4096 clauses')
        result = [list(dict.fromkeys(a + b)) for a in result for b in form]
        if sum(map(len, result)) > 100_000:
            raise Unsupported('CNF exceeds 100000 literals')
    return result


def relation(op, left, right, negated, ctx):
    text = bounded(f'({left})-({right})')
    if text not in ctx.polynomial_indices:
        ctx.polynomial_indices[text] = len(ctx.polynomials)
        ctx.polynomials.append(text)
    i = ctx.polynomial_indices[text]
    positive = {'=': ['='], 'distinct': ['>', '<'], '<': ['<'], '>': ['>'],
                '<=': ['<', '='], '>=': ['>', '=']}
    inverse = {'=': 'distinct', 'distinct': '=', '<': '>=', '>': '<=', '<=': '>', '>=': '<'}
    return [[(i, sign) for sign in positive[inverse[op] if negated else op]]]


def boolean(node, env, ctx, negated=False):
    if isinstance(node, String):
        raise Unsupported('string in Boolean formula')
    if isinstance(node, str):
        if node in env:
            term, scope = env[node]
            return boolean(term, scope, ctx, negated)
        if node in ('true', 'false'):
            return [] if (node == 'true') != negated else [[]]
        raise Unsupported('unsupported Boolean symbol: ' + node)
    if not node or not isinstance(node[0], str):
        raise Unsupported('unsupported Boolean term')
    op, args = node[0], node[1:]
    if op == 'let':
        return boolean(node[2], let_environment(node, env), ctx, negated)
    if op == '!':
        return boolean(node[1], env, ctx, negated)
    if op == 'not' and len(args) == 1:
        return boolean(args[0], env, ctx, not negated)
    if op in ('and', 'or'):
        if not args:
            raise Unsupported('empty Boolean application')
        combine = conjunction if (op == 'and') != negated else disjunction
        return combine(boolean(a, env, ctx, negated) for a in args)
    if op == '=>' and len(args) >= 2:
        return boolean(['or', *[['not', a] for a in args[:-1]], args[-1]], env, ctx, negated)
    if op == 'ite' and len(args) == 3:
        c, a, b = args
        return boolean(['or', ['and', c, a], ['and', ['not', c], b]], env, ctx, negated)
    if op in ('=', 'distinct', '<', '>', '<=', '>=') and len(args) >= 2:
        terms = [arithmetic(a, env, ctx)[0] for a in args]
        pairs = itertools.combinations(terms, 2) if op == 'distinct' else zip(terms, terms[1:])
        forms = (relation(op, a, b, negated, ctx) for a, b in pairs)
        return disjunction(forms) if negated else conjunction(forms)
    raise Unsupported('unsupported Boolean operator: ' + op)


def convert(text):
    ctx, env, assertions = Context(), {}, []
    checked, exited = False, False
    for command in sexpressions(text):
        if not isinstance(command, list) or not command:
            raise Unsupported('expected command')
        op, args = command[0], command[1:]
        if exited:
            raise Unsupported('command after exit')
        if op == 'set-info':
            if len(args) == 2 and args[0] == ':status': ctx.expected = str(args[1])
        elif op == 'set-logic':
            if args != ['QF_NRA']: raise Unsupported('logic is not QF_NRA')
        elif op == 'set-option':
            raise Unsupported('set-option is outside the adapter subset')
        elif op in ('declare-fun', 'declare-const'):
            if checked: raise Unsupported('declaration after check-sat')
            if op == 'declare-fun':
                if len(args) != 3 or args[1] != []: raise Unsupported('nonconstant declaration')
                name, sort = args[0], args[2]
            else:
                if len(args) != 2: raise Unsupported('invalid declaration')
                name, sort = args
            if sort != 'Real': raise Unsupported('non-Real declaration: ' + str(sort))
            if name in ctx.aliases or name in env: raise Unsupported('duplicate declaration')
            ctx.aliases[name] = f'v{len(ctx.names)}'
            ctx.names.append(name)
        elif op == 'define-fun':
            if checked or len(args) != 4 or args[1] != [] or args[2] not in ('Real', 'Bool'):
                raise Unsupported('only nullary Real/Bool definitions are supported')
            if args[0] in env or args[0] in ctx.aliases: raise Unsupported('duplicate definition')
            env[args[0]] = (args[3], dict(env))
        elif op == 'assert':
            if checked or len(args) != 1: raise Unsupported('incremental or invalid assertion')
            assertions.append(boolean(args[0], env, ctx))
        elif op == 'check-sat':
            if checked or args: raise Unsupported('requires one non-incremental check-sat')
            checked = True
        elif op == 'exit' and not args:
            exited = True
        else:
            raise Unsupported('unsupported command: ' + str(op))
    if not checked: raise Unsupported('missing check-sat')
    clauses = conjunction(assertions)
    if not ctx.polynomials: ctx.polynomials.append('0')
    if not clauses:
        ctx.polynomials.append('0')
        clauses = [[(len(ctx.polynomials)-1, '=')]]
    elif any(not clause for clause in clauses):
        ctx.polynomials.append('0')
        clauses = [[(len(ctx.polynomials)-1, '>')]]
    names = list(ctx.aliases.values()) or ['unused0']
    txt = ' '.join(names) + '\n\n' + '\n'.join(ctx.polynomials) + '\n\n'
    txt += '\n'.join(' | '.join(f'[{i},{op}]' for i, op in clause) for clause in clauses) + '\n'
    return txt, ctx, clauses


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('input', type=Path)
    parser.add_argument('--binary', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--mapping', type=Path)
    args = parser.parse_args()
    start = time.monotonic()
    try:
        txt, ctx, clauses = convert(args.input.read_text())
    except (Unsupported, RecursionError) as error:
        print('unsupported: ' + str(error), file=sys.stderr)
        return 3
    print(json.dumps({'conversion_seconds': time.monotonic()-start, 'variables': len(ctx.names),
                      'polynomials': len(ctx.polynomials), 'clauses': len(clauses)}), file=sys.stderr, flush=True)
    if args.output: args.output.write_text(txt)
    if args.mapping:
        args.mapping.write_text(json.dumps({
            'source_sha256': hashlib.sha256(args.input.read_bytes()).hexdigest(),
            'txt_sha256': hashlib.sha256(txt.encode()).hexdigest(),
            'variable_names': ctx.aliases, 'original_variables': ctx.names,
        }, ensure_ascii=False, indent=2) + '\n')
    if args.binary:
        result = subprocess.run([str(args.binary), '-'], input=txt, text=True)
        return result.returncode
    if not args.output: sys.stdout.write(txt)
    return 0


if __name__ == '__main__':
    sys.exit(main())
