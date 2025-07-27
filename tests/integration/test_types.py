import subprocess
import unittest

from tests.integration.helpers import AbleTestCase

TYPE_EXAMPLES = {
    'examples/types/string_type.abl': 'STRING\n',
    'examples/types/number_type.abl': 'NUMBER\n',
    'examples/types/boolean_type.abl': 'BOOLEAN\n',
    'examples/types/null_type.abl': 'NULL\n',
    'examples/types/object_type.abl': 'OBJECT\n',
    'examples/types/function_type.abl': 'FUNCTION\n',
    'examples/types/list_type.abl': 'LIST\n',
}

class TypeTests(AbleTestCase):
    def test_type_examples(self):
        for path, expected in TYPE_EXAMPLES.items():
            with self.subTest(example=path):
                output = self.run_script(path)
                self.assertEqual(output, expected)

    def test_type_invalid_no_args(self):
        with self.assertRaises(subprocess.CalledProcessError):
            self.run_script('examples/types/type_no_args.abl')

    def test_type_invalid_two_args(self):
        with self.assertRaises(subprocess.CalledProcessError):
            self.run_script('examples/types/type_two_args.abl')

if __name__ == '__main__':
    unittest.main()
