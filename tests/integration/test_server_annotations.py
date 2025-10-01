import unittest

from tests.integration.helpers import AbleTestCase


class ServerAnnotationTests(AbleTestCase):
    def test_server_route_metadata(self):
        output = self.run_script('examples/server/annotations_metadata.abl')
        self.assertEqual(
            output,
            '/api\n1\nGET:/\n1\nPOST:/items\n1\ntrue\n1\ntrue\ntrue\nfalse\n',
        )


if __name__ == '__main__':
    unittest.main()
