import subprocess
import unittest

from tests.integration.helpers import AbleTestCase

class ClassTests(AbleTestCase):
    def test_class_example_fails(self):
        with self.assertRaises(subprocess.CalledProcessError):
            self.run_script('examples/oop/class.abl')

    def test_inheritance(self):
        with self.assertRaises(subprocess.CalledProcessError):
            self.run_script('examples/oop/inheritance.abl')

    def test_static_method(self):
        with self.assertRaises(subprocess.CalledProcessError):
            self.run_script('examples/oop/static_method.abl')

if __name__ == '__main__':
    unittest.main()
