import unittest

from tests.integration.helpers import AbleTestCase


class NumberMethodTests(AbleTestCase):
    def test_decimal_methods(self):
        output = self.run_script('examples/numbers/decimal_methods.abl')
        self.assertEqual(
            output,
            '0.3\ntrue\n0.7\nfalse\n3\n0.75\n3\n1.5\n1\n1.23\n',
        )


if __name__ == '__main__':
    unittest.main()
