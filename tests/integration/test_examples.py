import unittest

from tests.integration.helpers import AbleTestCase

EXAMPLES = {
    'examples/variables/basic_assignment.abl': 'Daniel22.000000\n\n',
    'examples/objects/attr_set.abl': '30.000000\nWonderland\n',
    'examples/variables/simple_print.abl': 'Hello World!\n',
    'examples/objects/object_literal.abl': 'First name:Hof\n\n',
    'examples/functions/return_example.abl': 'test\n',
    'examples/functions/assign_from_return.abl': 'hello\n',
    'examples/variables/bool.abl': 'true\nfalse\n',
    'examples/variables/copy.abl': 'Alice\n',
    'examples/functions/greet.abl': 'AliceWonderland\n',
    'examples/functions/choose_first.abl': 'x\n',
    'examples/variables/math.abl': '5.000000\nHello World\n1.000000\n',
    'examples/variables/equality.abl': 'true\ntrue\nfalse\ntrue\n',
    'examples/variables/bool_func.abl': 'false\ntrue\nfalse\n',
    'examples/control/if_else.abl': 'B\n',
    'examples/variables/comparison.abl': 'true\ntrue\ntrue\ntrue\n',
    'examples/control/if_comp_lt.abl': 'lt\n',
    'examples/control/if_comp_gt.abl': 'gt\n',
    'examples/control/if_comp_lte.abl': 'lte\n',
    'examples/control/if_comp_gte.abl': 'gte\n',
}

class ExampleTests(AbleTestCase):
    def run_example(self, path):
        return self.run_script(path)

    def test_examples(self):
        for file, expected in EXAMPLES.items():
            with self.subTest(example=file):
                output = self.run_example(file)
                self.assertEqual(output, expected)

    def test_function_print(self):
        output = self.run_example('examples/functions/print_function.abl')
        self.assertRegex(output, r'^<function: greet at 0x[0-9a-fA-F]+>\n$')

if __name__ == '__main__':
    unittest.main()
