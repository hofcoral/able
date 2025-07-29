import unittest
from tests.integration.helpers import AbleTestCase

class BuiltinTests(AbleTestCase):
    def test_abs_builtin(self):
        output = self.run_script('examples/builtins/abs_example.abl')
        self.assertEqual(output, '5\n')

if __name__ == '__main__':
    unittest.main()
