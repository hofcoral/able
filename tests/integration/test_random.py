import unittest
from tests.integration.helpers import AbleTestCase

class RandomModuleTests(AbleTestCase):
    def test_randint_deterministic(self):
        output = self.run_script('examples/random/rand_example.abl')
        self.assertEqual(output, '8\n1\n')

    def test_choice_and_sample(self):
        output = self.run_script('examples/random/sample_example.abl')
        self.assertEqual(output, '4\n1\n1\n1\n')

if __name__ == '__main__':
    unittest.main()
