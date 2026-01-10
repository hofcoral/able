import unittest

from tests.integration.helpers import AbleTestCase


class StringModuleTests(AbleTestCase):
    def test_string_helpers(self):
        output = self.run_script('examples/string/basic.abl')
        self.assertEqual(
            output,
            'Able Lang\n3\na-b-c\nz-b-z\ntrue\ntrue\ntrue\nmix\nMIX\n',
        )


if __name__ == '__main__':
    unittest.main()
