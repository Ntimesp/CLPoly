"""原生 CAD 可执行文件的端到端测试；只依赖 Python 标准库。"""
import json
import re
from fractions import Fraction
import subprocess
import sys
import tarfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BINARY = Path(sys.argv.pop(1)).resolve()


class CadCliTests(unittest.TestCase):
    def invoke(self, text, *options):
        return subprocess.run([str(BINARY), *options, '-'], input=text, text=True,
                              capture_output=True, timeout=20)

    def test_sat_unsat_and_strict_relations(self):
        cases = [
            ('x\n\nx^2-2\nx\n\n[0,=]\n[1,>]\n', 'SAT'),
            ('x\n\nx^2+1\n\n[0,=]\n', 'UNSAT'),
            ('x\n\nx\n\n[0,>]\n[0,<]\n', 'UNSAT'),
            ('x\n\nx\n\n[0,>] | [0,<]\n', 'SAT'),
            ('x\n\n0\n\n[0,=]\n', 'SAT'),
            ('x\n\n0\n\n[0,>]\n', 'UNSAT'),
            ('y x\n\nx^2+y^2+1\n\n[0,<]\n', 'UNSAT'),
            ('y x\n\nx-y\nx+y\n\n[0,=]\n[1,>]\n', 'SAT'),
        ]
        for text, expected in cases:
            for options in [(), ('--reverse',)]:
                with self.subTest(text=text, options=options):
                    result = self.invoke(text, *options)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, expected + '\n')
                    self.assertEqual(result.stderr, '')

    def test_exact_sample_certificate(self):
        text = (ROOT / 'examples/cad/sat.txt').read_text()
        result = self.invoke(text, '--sample')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(result.stdout.startswith('SAT\nsample_variables: x\nequations:\n'))
        # 定义链可以是投影因子的乘积，并不要求等于原输入方程。
        self.assertRegex(result.stdout, r'equations:\n[^\n]+ = 0\n')
        self.assertIn('isolating_intervals:\nx in (', result.stdout)
        self.assertIn('\nshifts:', result.stdout)
        bracket = re.search(r'x in \(([^,]+), ([^)]+)\)', result.stdout)
        self.assertIsNotNone(bracket)
        left, right = map(Fraction, bracket.groups())
        self.assertGreater(left, 0)
        self.assertLess(left*left, 2)
        self.assertGreater(right*right, 2)
        two_variables = 'x y\n\nx-1\ny-2\n\n[0,=]\n[1,=]\n'
        for options, order in [(('--sample',), 'y x'), (('--sample', '--reverse'), 'x y')]:
            answer = self.invoke(two_variables, *options)
            self.assertEqual(answer.returncode, 0, answer.stderr)
            self.assertIn('sample_variables: ' + order + '\n', answer.stdout)
            for name, exact in [('x', 1), ('y', 2)]:
                bracket = re.search(name + r' in \(([^,]+), ([^)]+)\)', answer.stdout)
                self.assertIsNotNone(bracket)
                left, right = map(Fraction, bracket.groups())
                self.assertLess(left, exact)
                self.assertGreater(right, exact)
        result = self.invoke('x\n\nx^2+1\n\n[0,=]\n', '--sample')
        self.assertEqual(result.stdout, 'UNSAT\n')

    def test_file_and_standard_input(self):
        result = subprocess.run([str(BINARY), str(ROOT / 'examples/cad/sat.txt')],
                                text=True, capture_output=True, timeout=20)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, 'SAT\n')
        result = self.invoke('\ufeffx\r\n\r\nx^2-2\r\n\r\n[0,=]\r\n')
        self.assertEqual(result.stdout, 'SAT\n')

    def test_bad_input_has_no_answer(self):
        texts = ['', 'x\n\nx/0\n\n[0,=]', 'x\n\ny\n\n[0,=]',
                 'x\n\nx\n\n[-1,=]', 'x\n\nx\n\n[1,=]',
                 'x x\n\nx\n\n[0,=]', 'x\n\nx^-1\n\n[0,=]']
        for text in texts:
            with self.subTest(text=text):
                result = self.invoke(text)
                self.assertEqual(result.returncode, 2)
                self.assertEqual(result.stdout, '')
                self.assertTrue(result.stderr.startswith('clpoly-cad:'))

    def test_arguments_and_help(self):
        for args in [[], ['--unknown'], ['--sample', '--check', '-'], ['a', 'b'],
                     ['/nonexistent/clpoly-formula.txt']]:
            with self.subTest(args=args):
                result = subprocess.run([str(BINARY), *args], text=True, capture_output=True, timeout=10)
                self.assertEqual(result.returncode, 2)
                self.assertEqual(result.stdout, '')
        result = subprocess.run([str(BINARY), '--help'], text=True, capture_output=True, timeout=10)
        self.assertEqual(result.returncode, 0)
        self.assertIn('Usage:', result.stdout)

    def test_all_72_archived_txt_inputs(self):
        archive = ROOT / 'docs/cad/reports/three-datasets/input-snapshot.tar.gz'
        count = 0
        with tarfile.open(archive) as source:
            for member in source.getmembers():
                if not member.name.endswith('.txt'):
                    continue
                text = source.extractfile(member).read().decode('utf-8')
                with self.subTest(name=member.name):
                    result = self.invoke(text, '--check')
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, 'VALID\n')
                count += 1
        self.assertEqual(count, 72)

    def test_archived_easy_queries(self):
        # 直接读取归档 TXT 并求解，不使用已保存的多项式项表作输入。
        cases = json.loads((ROOT / 'docs/cad/reports/three-datasets/cases.json').read_text())
        with tarfile.open(ROOT / 'docs/cad/reports/three-datasets/input-snapshot.tar.gz') as source:
            for case in cases[:8]:
                text = source.extractfile('CAD测试题集_三数据集/' + case['source_path']).read().decode('utf-8')
                with self.subTest(id=case['id']):
                    result = self.invoke(text)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout.strip(), case['expected'].upper())


if __name__ == '__main__':
    unittest.main(verbosity=2)
