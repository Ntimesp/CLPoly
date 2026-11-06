import json
import tempfile
import unittest
from pathlib import Path

import check_witness


class WitnessTests(unittest.TestCase):
    def verify_sample(self, equation, interval):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root/'source.smt2'
            source.write_text('(set-logic QF_NRA)(declare-fun x () Real)(assert (and (= (* x x) 2) (> x 0)))(check-sat)')
            mapping = root/'mapping.json'
            mapping.write_text(json.dumps({'variable_names': {'x':'v0'}}))
            sample = root/'sample.txt'
            sample.write_text('SAT\nsample_variables: v0\nequations:\n'+equation+' = 0\nisolating_intervals:\nv0 in '+interval+'\nshifts: 4\n')
            return check_witness.check(source,sample,mapping)

    def test_valid_product_chain_certificate(self):
        result = self.verify_sample('v0^3-2*v0','(5/4, 3/2)')
        self.assertEqual(result['status'],'valid')
        self.assertEqual(result['certificate_nonempty'],'sat')
        self.assertEqual(result['certificate_and_not_original'],'unsat')

    def test_wrong_selected_point_is_detected(self):
        result = self.verify_sample('v0+1','(-5/4, -3/4)')
        self.assertEqual(result['status'],'invalid')
        self.assertEqual(result['certificate_and_not_original'],'sat')

    def test_empty_certificate_is_not_accepted_vacuously(self):
        result = self.verify_sample('v0^2+1','(0, 1)')
        self.assertEqual(result['status'],'invalid')
        self.assertEqual(result['certificate_nonempty'],'unsat')

    def test_bad_variable_mapping_is_rejected(self):
        with self.assertRaises(ValueError):
            check_witness.certificate('SAT\nsample_variables: v1\nequations:\nv1 = 0\nisolating_intervals:\nv1 in (-1, 1)\n',{'x':'v0'})


if __name__=='__main__':unittest.main(verbosity=2)
