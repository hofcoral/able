import unittest
from tests.integration.helpers import AbleTestCase

class RandomModuleTests(AbleTestCase):
    def test_randint_deterministic(self):
        output = self.run_script('examples/random/rand_example.abl')
        self.assertEqual(output, '8\n5\n')

    def test_choice_and_sample(self):
        output = self.run_script('examples/random/sample_example.abl')
        self.assertEqual(output, '4\n1\n4\n2\n')

    def test_random_validation(self):
        output = self.run_script('examples/random/validation_example.abl')
        self.assertEqual(output, 'true\ntrue\ntrue\n')

if __name__ == '__main__':
    unittest.main()
