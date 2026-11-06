import ast
import itertools
import json
import hashlib
import tempfile
import subprocess
import sys
import unittest
from fractions import Fraction
from pathlib import Path

import adapter
import benchmark
import z3


def source(formula, declarations='(declare-fun x () Real)(declare-fun y () Real)'):
    return '(set-logic QF_NRA)' + declarations + '(assert ' + formula + ')(check-sat)(exit)'


def infix_z3(text, variables):
    def visit(node):
        if isinstance(node, ast.Constant) and type(node.value) is int:
            return z3.RealVal(node.value)
        if isinstance(node, ast.Name): return variables[node.id]
        if isinstance(node, ast.UnaryOp):
            q = visit(node.operand)
            if isinstance(node.op, ast.USub): return -q
            if isinstance(node.op, ast.UAdd): return q
        if isinstance(node, ast.BinOp):
            a, b = visit(node.left), visit(node.right)
            if isinstance(node.op, ast.Add): return a+b
            if isinstance(node.op, ast.Sub): return a-b
            if isinstance(node.op, ast.Mult): return a*b
            if isinstance(node.op, ast.Div): return a/b
            if isinstance(node.op, ast.Pow): return a**b
        raise ValueError(ast.dump(node))
    return visit(ast.parse(text.replace('^', '**'), mode='eval').body)


def converted_formula(ctx, clauses):
    variables = {alias: z3.Real(name) for name, alias in ctx.aliases.items()}
    polynomials = [infix_z3(p, variables) for p in ctx.polynomials]
    return z3.And(*[z3.Or(*[{'=': polynomials[i] == 0, '>': polynomials[i] > 0,
                             '<': polynomials[i] < 0}[op] for i, op in c]) for c in clauses])


class AdapterTests(unittest.TestCase):
    def equivalent(self, text):
        _, ctx, clauses = adapter.convert(text)
        original = z3.And(*z3.parse_smt2_string(text))
        solver = z3.SolverFor('QF_NRA')
        solver.set(timeout=10000)
        solver.add(z3.Xor(original, converted_formula(ctx, clauses)))
        self.assertEqual(solver.check(), z3.unsat)

    def test_all_relations_and_negation(self):
        for op, negation in itertools.product(['=', 'distinct', '>', '<', '>=', '<='], [False, True]):
            formula = f'({op} (+ (* x x) 1.25) (/ y 3.0) 2)'
            if negation: formula = '(not ' + formula + ')'
            with self.subTest(op=op, negated=negation): self.equivalent(source(formula))

    def test_booleans(self):
        for formula in ['true', 'false',
                        '(or (and (> x 0) (< y 1)) (not (= x y)))',
                        '(not (or (>= x 0) (<= y 1)))',
                        '(=> (> x 0) (< x y) (> y 0))',
                        '(ite (> x 0) (= y 1) (= y 2))',
                        '(not (ite (> x 0) (= y 1) (= y 2)))']:
            with self.subTest(formula=formula): self.equivalent(source(formula))

    def test_let_is_simultaneous_and_lexical(self):
        self.equivalent(source('(let ((x y) (y x)) (= x (+ y 1)))'))
        self.equivalent(source('(let ((a (+ x 1))) (let ((x 0) (b a)) (= b (+ x 1))))'))
        self.equivalent(source('(let ((p (> x 0))) (or p (let ((p (< y 0))) (not p))))'))

    def test_definitions_annotations_and_quoted_names(self):
        text = ('(set-logic QF_NRA)(declare-fun |x with space| () Real)'
                '(define-fun p () Bool (> |x with space| 1.1))'
                '(define-fun q () Bool (not p))'
                '(assert (! (or p q) :named named_assertion))(check-sat)(exit)')
        self.equivalent(text)

    def test_exact_constants_and_multiple_assertions(self):
        self.equivalent(source('(= x (/ (- 12345678901234567890123456789.125) 7))'))
        self.equivalent('(set-logic QF_NRA)(declare-const x Real)(assert (> x 0))(assert (< x 1))(check-sat)')
        self.equivalent('(set-logic QF_NRA)(assert (= 1.0 1))(check-sat)')

    def test_standalone_parser_writes_renamed_txt_and_mapping(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source_file, target, mapping = root/'case.smt2', root/'case.txt', root/'case.names.json'
            source_file.write_text(source('(= |weird x| 0.1)', '(declare-fun |weird x| () Real)'))
            call = subprocess.run([sys.executable, str(Path(adapter.__file__)), str(source_file),
                                   '--output',str(target),'--mapping',str(mapping)],
                                  capture_output=True,text=True,timeout=10)
            self.assertEqual(call.returncode,0,call.stderr)
            self.assertEqual(call.stdout,'')
            self.assertTrue(target.read_text().startswith('v0\n\n'))
            names = json.loads(mapping.read_text())
            self.assertEqual(names['variable_names'],{'weird x':'v0'})
            self.assertEqual(names['source_sha256'],hashlib.sha256(source_file.read_bytes()).hexdigest())
            self.assertEqual(names['txt_sha256'],hashlib.sha256(target.read_bytes()).hexdigest())

    def test_metadata_never_changes_formula(self):
        text = source('(= x y)')
        a = adapter.convert('(set-info :status sat)' + text)
        b = adapter.convert('(set-info :status unsat)' + text)
        self.assertEqual(a[0], b[0])
        self.assertNotEqual(a[1].expected, b[1].expected)

    def test_unsupported_is_explicit(self):
        cases = [source('(and)'), source('(or)'), source('(= x (/ y 0))'), source('(= x (/ y x))'),
                 source('(> (sin x) 0)'), source('(= x (ite (> y 0) y 0))'),
                 source('b', '(declare-fun b () Bool)'),
                 source('(= x 1)', '(declare-fun x () Int)'),
                 source('(> x 0)').replace('(check-sat)', '(check-sat)(check-sat)'),
                 source('(> x 0)').replace('(check-sat)', '(push 1)(check-sat)'),
                 source('(> x 0)').replace('(set-logic QF_NRA)', '(set-logic QF_LRA)'),
                 '(set-logic QF_NRA)(assert true)', '(assert (and true)']
        for text in cases:
            with self.subTest(text=text), self.assertRaises(adapter.Unsupported): adapter.convert(text)

    def test_cnf_distribution_limit(self):
        text = source('(or ' + ' '.join('(and (> x 0) (< y 0))' for _ in range(14)) + ')')
        with self.assertRaises(adapter.Unsupported): adapter.convert(text)

    def test_timeout_and_unsupported_results(self):
        timed = benchmark.run_process([sys.executable, '-c', 'import time; time.sleep(5)'], .1)
        self.assertEqual(timed['status'], 'timeout')
        self.assertLess(timed['wall_seconds'], 2)
        failed = benchmark.run_process([sys.executable, '-c', 'print("UNSAT"); raise SystemExit(1)'], 2)
        self.assertEqual(failed['status'], 'error')
        skipped = benchmark.run_process([sys.executable, '-c', 'import sys; print("unsupported: sort", file=sys.stderr); sys.exit(3)'], 2)
        self.assertEqual(skipped['status'], 'unsupported')


if __name__ == '__main__':
    unittest.main(verbosity=2)
