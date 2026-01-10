import unittest

from tests.integration.helpers import AbleTestCase


class MathEdgeCaseTests(AbleTestCase):
    def test_math_edge_cases(self):
        output = self.run_script('examples/math/edge_cases.abl')
        self.assertEqual(output, 'true\ntrue\ntrue\ntrue\ntrue\n')


if __name__ == '__main__':
    unittest.main()
