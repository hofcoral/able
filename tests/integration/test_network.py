import unittest

from tests.integration.helpers import AbleTestCase


class NetworkRequestTests(AbleTestCase):
    def test_http_requests(self):
        output = self.run_script('examples/network/http_requests.abl')
        lines = [line for line in output.strip().splitlines() if line]
        self.assertEqual(len(lines), 13)

        self.assertEqual(lines[0], '200')
        self.assertEqual(lines[1], 'true')
        self.assertEqual(lines[2], 'https://httpbin.org/get')
        self.assertEqual(lines[3], 'true')
        self.assertEqual(lines[4], 'true')

        self.assertEqual(lines[5], '200')
        self.assertEqual(lines[6], 'true')
        self.assertEqual(lines[7], 'true')
        self.assertEqual(lines[8], 'true')
        self.assertEqual(lines[9], 'POST')

        self.assertEqual(lines[10], '200')
        self.assertEqual(lines[11], 'true')
        self.assertEqual(lines[12], 'GET')


if __name__ == '__main__':
    unittest.main()
