"""实验工具回归；可选 Z3 对照和 CLPOLY_CAD_DRIVER 指定的 C++ 驱动。

python3 docs/cad/reports/three-datasets/test_tools.py
"""
import hashlib
import importlib.util
import json
import os
import subprocess
import tarfile
import tempfile
import unittest
from fractions import Fraction as Q
from pathlib import Path

from linear_preprocess import preprocess
import check_witnesses as witness

HERE = Path(__file__).resolve().parent
try:
    import z3
    import run as experiment
except ImportError:
    z3 = None


def case(variables, polynomials, clauses):
    return dict(id='regression', variables=variables, polynomials=polynomials, clauses=clauses)


def term(exponents, numerator=1, denominator=1):
    return dict(exp=exponents, num=numerator, den=denominator)


def value(polynomial, values):
    total = Q(0)
    for t in polynomial:
        product = Q(t['num'], t['den'])
        for x, e in zip(values, t['exp']):
            product *= x ** e
        total += product
    return total


def truth(formula, values):
    signs = [value(p, values) for p in formula['polynomials']]
    return all(any({'=': signs[i] == 0, '>': signs[i] > 0, '<': signs[i] < 0}[op]
                   for i, op in clause) for clause in formula['clauses'])


class PreprocessTests(unittest.TestCase):
    def test_empty_true_and_empty_clause_false(self):
        self.assertEqual(preprocess(case([], [], []))['preprocess_status'], 'sat')
        self.assertEqual(preprocess(case([], [], [[]]))['preprocess_status'], 'unsat')

    def test_reference_is_optional_metadata(self):
        source = case(['x'], [[term([1])]], [[[0, '=']]])
        plain = preprocess(source)
        self.assertEqual(plain['preprocess_status'], 'sat')
        annotated = preprocess(dict(source, z3_status='unsat'))
        for key in plain.keys() - {'preprocess_seconds'}:
            self.assertEqual(plain[key], annotated[key])
        self.assertEqual(annotated['z3_status'], 'unsat')

    def test_duplicate_terms_are_summed(self):
        source = case(['x'], [[term([1]), term([1], -1)]], [[[0, '>']]])
        self.assertEqual(preprocess(source)['preprocess_status'], 'unsat')

    def test_disjunction_and_variable_pivot_are_protected(self):
        source = case(['x', 'y'], [[term([1, 0])], [term([0, 1])]], [[[0, '='], [1, '=']]])
        result = preprocess(source)
        self.assertEqual(result['remaining_dimension'], 2)
        source = case(['a', 'x'], [[term([1, 1])]], [[[0, '=']]])
        self.assertEqual(preprocess(source)['remaining_dimension'], 2)

    def test_negative_pivot_and_reconstruction(self):
        source = case(['x', 'y'], [[term([1, 0], -2), term([0, 1]), term([0, 0], 1)],
                                  [term([1, 0], 1), term([0, 0], -2)]], [[[0, '=']], [[1, '<']]])
        result = preprocess(source)
        self.assertEqual(result['remaining_dimension'], 1)
        for u in [Q(-5), Q(-1, 3), Q(0), Q(7, 2), Q(10)]:
            restored = [value(p, [u]) for p in result['reconstruction']]
            self.assertEqual(truth(source, restored), truth(result, [u]))

    def test_contradiction_and_new_unit_clause(self):
        source = case(['x'], [[term([1])], [term([1]), term([0], -1)]], [[[0, '=']], [[1, '=']]])
        self.assertEqual(preprocess(source)['preprocess_status'], 'unsat')
        source = case(['x', 'y'], [[term([1, 0]), term([0, 0], -1)],
                                  [term([1, 0])], [term([0, 1])]],
                      [[[0, '=']], [[1, '='], [2, '=']]])
        result = preprocess(source)
        self.assertEqual(result['preprocess_status'], 'sat')
        self.assertEqual([value(p, []) for p in result['reconstruction']], [Q(1), Q(0)])

    def test_invalid_schema(self):
        source = case(['x'], [[term([1])]], [[[0, '=']]])
        changes = [dict(variables=['x', 'x']), dict(clauses=[[[-1, '=']]]),
                   dict(clauses=[[[1, '=']]]), dict(clauses=[[[0, '!=']]]),
                   dict(polynomials=[[term([-1])]]), dict(polynomials=[[term([1], 1, 0)]]),
                   dict(polynomials=[[term([])]]), dict(polynomials=[[term([True])]])]
        for change in changes:
            with self.subTest(change=change), self.assertRaises(ValueError):
                preprocess(dict(source, **change))


class EvidenceTests(unittest.TestCase):
    def test_historical_hashes(self):
        hashes = json.loads((HERE / 'artifact-sha256.json').read_text())
        with tarfile.open(HERE / 'build-snapshot.tar.gz') as snapshot:
            for name, expected in hashes.items():
                if name in ['driver.cc', 'linear_preprocess.py', 'run.py', 'profile.py', 'check_witnesses.py']:
                    data = snapshot.extractfile('docs/cad/reports/three-datasets/' + name).read()
                else:
                    data = (HERE / name).read_bytes()
                with self.subTest(name=name):
                    self.assertEqual(hashlib.sha256(data).hexdigest(), expected)

        manifest = json.loads((HERE / 'manifest.json').read_text())['sha256']
        with tarfile.open(HERE / 'input-snapshot.tar.gz') as inputs, \
                tarfile.open(HERE / 'build-snapshot.tar.gz') as build:
            for name, expected in manifest.items():
                if '/CLPoly/' in name:
                    member = name.split('/CLPoly/', 1)[1]
                    archive = build
                else:
                    member = 'CAD测试题集_三数据集/' + name.split('/CAD测试题集_三数据集/', 1)[1]
                    archive = inputs
                with self.subTest(member=member):
                    self.assertEqual(hashlib.sha256(archive.extractfile(member).read()).hexdigest(), expected)

    def test_exact_radical_arithmetic(self):
        self.assertEqual(witness.parse('sqrt(8)-2*sqrt(2)'), {})
        self.assertEqual(witness.parse('sqrt(6)*sqrt(10)'), {15: Q(2)})
        self.assertEqual(witness.sign(witness.parse('sqrt(2)-7/5')), 1)
        self.assertEqual(witness.sign(witness.parse('7/5-sqrt(2)')), -1)
        for text in ['sqrt(-1)', 'sqrt(0)', '1/0', '1/sqrt(2)', 'sqrt(2,3)', '1.5']:
            with self.subTest(text=text), self.assertRaises(ValueError):
                witness.parse(text)

    def test_all_saved_sat_witnesses(self):
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory)
            for name in ['sources.json', 'cases.json']:
                (out / name).write_bytes((HERE / name).read_bytes())
            witness.main(out)
            results = json.loads((out / 'exact-witnesses.json').read_text())
            self.assertEqual(len(results), 25)
            self.assertTrue(all(r['valid'] for r in results))

    @unittest.skipUnless(z3, 'optional z3-solver is unavailable')
    def test_parser_exactness_and_rejections(self):
        self.assertEqual(experiment.parse_polynomial('(x+1)^2/3', ['x']),
                         {(2,): Q(1, 3), (1,): Q(2, 3), (0,): Q(1, 3)})
        for polynomial in [[term([1, 0])], [term([-1])], [term([1], 1, 0)]]:
            with self.assertRaises(ValueError):
                experiment.polynomial_z3(polynomial, [z3.Real('x')])
        for text in ['x^-1', 'x^21', 'x^y', 'x^1.5', 'x/0', 'x/(x+1)', 'sqrt(x)', '0.5*x']:
            with self.subTest(text=text), self.assertRaises(ValueError):
                experiment.parse_polynomial(text, ['x'])

    @unittest.skipUnless(z3, 'optional z3-solver is unavailable')
    def test_all_72_preprocessing_maps_both_directions(self):
        cases = json.loads((HERE / 'cases.json').read_text())
        self.assertEqual(len(cases), 72)
        for source in cases:
            with self.subTest(id=source['id']):
                result = preprocess(source)
                u = [z3.Real(f'u{i}') for i in range(len(result['variables']))]
                restored = [experiment.polynomial_z3(p, u) for p in result['reconstruction']]
                solver = z3.SolverFor('QF_NRA')
                solver.set(timeout=15000)
                solver.add(z3.Xor(experiment.formula(source, restored), experiment.formula(result, u)))
                self.assertEqual(solver.check(), z3.unsat)
                # 检查全部原解可恢复，独立于映射上等价的第一项检查。
                x = [z3.Real(f'x{i}') for i in range(len(source['variables']))]
                remaining = [x[source['variables'].index(v)] for v in result['variables']]
                recovered = [experiment.polynomial_z3(p, remaining) for p in result['reconstruction']]
                solver = z3.SolverFor('QF_NRA')
                solver.set(timeout=15000)
                solver.add(experiment.formula(source, x), z3.Or(*[a != b for a, b in zip(x, recovered)]))
                self.assertEqual(solver.check(), z3.unsat)

    def test_profile_replacement_requires_unique_site(self):
        spec = importlib.util.spec_from_file_location('cad_profile', HERE / 'profile.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        self.assertEqual(module.replace('        a\n            b\n', '    a\n        b', '    c'), '        c\n')
        for text in ['absent', 'a\na\n']:
            with self.assertRaises(ValueError):
                module.replace(text, 'a', 'c')


@unittest.skipUnless(os.environ.get('CLPOLY_CAD_DRIVER'), 'set CLPOLY_CAD_DRIVER for C++ input tests')
class DriverTests(unittest.TestCase):
    def invoke(self, text, *args):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'input.cad'
            path.write_text(text)
            return subprocess.run([os.environ['CLPOLY_CAD_DRIVER'], str(path), *args],
                                  text=True, capture_output=True, timeout=15)

    def test_invalid_input_is_rejected(self):
        for text in ['', '-1 0 0', '0 0', '1 0 0 x junk', '0 1 0 1 1 0',
                     '1 1 0 x 1 1 1 -1', '1 1 0 x 1 1 1 4294967296',
                     '0 0 1 1 0 =', '0 1 1 0 1 0 ?', '0 1 1 0 1 0 ==',
                     '0 1 0 1 abc 1', '18446744073709551616 0 0']:
            with self.subTest(text=text):
                self.assertEqual(self.invoke(text).returncode, 1)
        self.assertEqual(self.invoke('0 0 0', 'typo').returncode, 2)
        missing = subprocess.run([os.environ['CLPOLY_CAD_DRIVER'], '/nonexistent/clpoly-input.cad'],
                                 capture_output=True, timeout=15)
        self.assertEqual(missing.returncode, 1)

    def test_empty_and_univariate_formulas(self):
        examples = [('0 0 0', 'sat'), ('0 0 1 0', 'unsat'),
                    ('1 1 1 x 2 1 1 2 -2 1 0 1 0 =', 'sat'),
                    ('1 1 1 x 2 1 1 2 1 1 0 1 0 <', 'unsat')]
        for text, expected in examples:
            for args in [(), ('reverse',)]:
                with self.subTest(text=text, args=args):
                    result = self.invoke(text, *args)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertIn('status=' + expected, result.stdout)


if __name__ == '__main__':
    unittest.main(verbosity=2)
