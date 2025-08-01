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

    def test_math_round(self):
        output = self.run_script('examples/modules/import_round.abl')
        self.assertEqual(output, '3\n3\n')

    def test_custom_module(self):
        output = self.run_script('examples/modules/import_custom.abl')
        self.assertEqual(output, 'Hello, Codex\n')

    def test_class_import(self):
        output = self.run_script('examples/modules/import_class.abl')
        self.assertEqual(output, 'Hi Alice\n')

if __name__ == '__main__':
    unittest.main()
