import unittest
import subprocess
from pathlib import Path

EXE = Path('build/able_exe')
EXAMPLES = {
    'examples/variables/basic_assignment.abl': 'Daniel 22.000000\n\n',
    'examples/objects/attr_set.abl': '30.000000\nWonderland\n',
    'examples/variables/simple_print.abl': 'Hello World!\n',
    'examples/objects/object_literal.abl': 'First name: Hof\n\n',
    'examples/functions/return_example.abl': 'test\n',
    'examples/functions/assign_from_return.abl': 'hello\n',
    'examples/variables/bool.abl': 'true\nfalse\n',
    'examples/variables/copy.abl': 'Alice\n',
    'examples/functions/greet.abl': 'Alice Wonderland\n',
    'examples/functions/choose_first.abl': 'x\n',
}

class ExampleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        subprocess.run(['make'], check=True)
        if not EXE.exists():
            raise RuntimeError('Executable not built')

    def run_example(self, path):
        result = subprocess.run([str(EXE), path], check=True, capture_output=True, text=True)
        return result.stdout

    def test_examples(self):
        for file, expected in EXAMPLES.items():
            with self.subTest(example=file):
                output = self.run_example(file)
                self.assertEqual(output, expected)

if __name__ == '__main__':
    unittest.main()
