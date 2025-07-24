import unittest
import subprocess
from pathlib import Path

EXE = Path('build/able_exe')
EXAMPLES = {
    'examples/example.abl': 'Daniel 22.000000\n\n',
    'examples/attr_set.abl': '30.000000\nWonderland\n',
    'examples/main.abl': 'Hello World!\n',
    'examples/objects.abl': 'First name: Hof\n\n',
    'examples/return_example.abl': 'test\n',
    'examples/assign_from_return.abl': 'hello\n',
    'examples/bool.abl': 'true\nfalse\n',
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
