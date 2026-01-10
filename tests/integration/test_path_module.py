import unittest

from tests.integration.helpers import AbleTestCase


class PathModuleTests(AbleTestCase):
    def test_path_helpers(self):
        output = self.run_script('examples/path/basic.abl')
        self.assertEqual(output, '/\n/api\n/api/users\n/\n/api\n')


if __name__ == '__main__':
    unittest.main()
