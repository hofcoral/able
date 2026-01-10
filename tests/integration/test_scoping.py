import unittest

from tests.integration.helpers import AbleTestCase


class ScopingTests(AbleTestCase):
    def test_function_scope_does_not_mutate_outer(self):
        output = self.run_script('examples/variables/function_scope.abl')
        self.assertEqual(output, '2\n5\n')


if __name__ == '__main__':
    unittest.main()
