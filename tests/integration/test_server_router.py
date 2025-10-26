import unittest

from tests.integration.helpers import AbleTestCase


class ServerRouterTests(AbleTestCase):
    def test_build_routes_binds_handlers(self):
        output = self.run_script('examples/server/router_build.abl')
        self.assertEqual(
            output,
            '2\nGET /api\nuser:GET\nPOST /api/users\nuser:POST\n',
        )

    def test_create_router_exposes_metadata_and_matching(self):
        output = self.run_script('examples/server/router_runtime.abl')
        self.assertEqual(
            output,
            'controller\nGET /api\n1\n0\nPOST /api/items\n1\ntrue\n',
        )


if __name__ == '__main__':
    unittest.main()
