import unittest

from tests.integration.helpers import AbleTestCase


class AnnotationTests(AbleTestCase):
    def test_custom_modifiers_and_decorators(self):
        output = self.run_script('examples/annotations/basic.abl')
        self.assertEqual(output, 'greeted: Hello World\ntrue\nfunction\npong\n')


if __name__ == '__main__':
    unittest.main()
