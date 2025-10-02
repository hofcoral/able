import unittest

from tests.integration.helpers import AbleTestCase


class ServerRouterTests(AbleTestCase):
    def test_build_routes_binds_handlers(self):
        output = self.run_script('examples/server/router_build.abl')
        self.assertEqual(
            output,
            '2\nGET /api\nuser:GET\nPOST /api/users\nuser:POST\n',
        )


if __name__ == '__main__':
    unittest.main()
