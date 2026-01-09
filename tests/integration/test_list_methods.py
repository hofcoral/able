import unittest

from tests.integration.helpers import AbleTestCase


class ListMethodTests(AbleTestCase):
    def test_list_set_negative_indexes(self):
        output = self.run_script('examples/variables/list_set_negative.abl')
        self.assertEqual(output, '7\n99\n')


if __name__ == '__main__':
    unittest.main()
