import unittest

from tests.integration.helpers import AbleTestCase

EXAMPLES = {
    'examples/variables/basic_assignment.abl': 'Daniel22\n\n',
    'examples/objects/attr_set.abl': '30\nWonderland\n',
    'examples/variables/simple_print.abl': 'Hello World!\n',
    'examples/objects/object_literal.abl': 'First name:Hof\n\n',
    'examples/functions/return_example.abl': 'test\n',
    'examples/functions/assign_from_return.abl': 'hello\n',
    'examples/variables/bool.abl': 'true\nfalse\n',
    'examples/variables/copy.abl': 'Alice\n',
    'examples/functions/greet.abl': 'AliceWonderland\n',
    'examples/functions/choose_first.abl': 'x\n',
    'examples/functions/fib_recursion.abl': '8\n',
    'examples/variables/math.abl': '5\nHello World\n1\n',
    'examples/variables/equality.abl': 'true\ntrue\nfalse\ntrue\n',
    'examples/variables/bool_func.abl': 'false\ntrue\nfalse\n',
    'examples/control/if_else.abl': 'B\n',
    'examples/variables/comparison.abl': 'true\ntrue\ntrue\ntrue\n',
    'examples/control/if_comp_lt.abl': 'lt\n',
    'examples/control/if_comp_gt.abl': 'gt\n',
    'examples/control/if_comp_lte.abl': 'lte\n',
    'examples/control/if_comp_gte.abl': 'gte\n',
    'examples/types/string_type.abl': 'STRING\n',
    'examples/types/number_type.abl': 'NUMBER\n',
    'examples/types/boolean_type.abl': 'BOOLEAN\n',
    'examples/types/null_type.abl': 'NULL\n',
    'examples/types/object_type.abl': 'OBJECT\n',
    'examples/types/function_type.abl': 'FUNCTION\n',
    'examples/variables/list_ops.abl': '1\n2\n3\n',
    'examples/control/for_loop.abl': '1\n2\n3\n',
    'examples/control/for_number.abl': '0\n1\n2\n3\n4\n',
    'examples/control/while_loop.abl': '0\n1\n2\n',
    'examples/control/break_continue.abl': '1\n2\n',
    'examples/variables/increment.abl': '0\n1\n',
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
