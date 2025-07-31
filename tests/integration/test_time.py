import unittest
from tests.integration.helpers import AbleTestCase

class TimeModuleTests(AbleTestCase):
    def test_sleep(self):
        output = self.run_script('examples/time/sleep_example.abl')
        self.assertEqual(output, 'true\n')

if __name__ == '__main__':
    unittest.main()
