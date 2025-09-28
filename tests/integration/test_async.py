import unittest

from tests.integration.helpers import AbleTestCase


class AsyncRuntimeTests(AbleTestCase):
    def test_async_time_behavior(self):
        output = self.run_script('examples/async/await_time.abl')
        expected = ('true\n' * 10)
        self.assertEqual(output, expected)


if __name__ == '__main__':
    unittest.main()
