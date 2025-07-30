import unittest
from tests.integration.helpers import AbleTestCase

class ImportTests(AbleTestCase):
    def test_import_module(self):
        output = self.run_script('examples/modules/import_module.abl')
        self.assertEqual(output, '5\n')

    def test_from_import(self):
        output = self.run_script('examples/modules/from_import.abl')
        self.assertEqual(output, '5\n')

    def test_math_sqrt(self):
        output = self.run_script('examples/modules/import_sqrt.abl')
        self.assertEqual(output, '3\n')

if __name__ == '__main__':
    unittest.main()
