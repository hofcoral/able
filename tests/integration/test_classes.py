import unittest

from tests.integration.helpers import AbleTestCase

class ClassTests(AbleTestCase):
    def test_class_example(self):
        output = self.run_script('examples/oop/class.abl')
        self.assertEqual(output, 'Hello from Donald!\n')

    def test_inheritance(self):
        output = self.run_script('examples/oop/inheritance.abl')
        self.assertEqual(output, 'hello\nhello\nHELLO\n')

    def test_static_method(self):
        output = self.run_script('examples/oop/static_method.abl')
        self.assertEqual(output, '5\n')

if __name__ == '__main__':
    unittest.main()
