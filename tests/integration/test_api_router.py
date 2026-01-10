import unittest

from tests.integration.helpers import AbleTestCase


class ApiRouterTests(AbleTestCase):
    def test_build_routes(self):
        output = self.run_script('examples/api/router_build.abl')
        self.assertEqual(
            output,
            '2\nGET /api\nuser:GET\nPOST /api/users\nuser:POST\n',
        )

    def test_route_path_normalization(self):
        output = self.run_script('examples/api/router_normalize.abl')
        self.assertEqual(output, '/api\n/\n')

    def test_route_method_validation(self):
        output = self.run_script('examples/api/router_validate.abl')
        self.assertEqual(
            output,
            '1\n1\ninvalid_method\nFETCH\n/bad\n',
        )

    def test_duplicate_routes(self):
        output = self.run_script('examples/api/router_duplicate.abl')
        self.assertEqual(
            output,
            '1\n1\nduplicate_route\nGET\n/api/items\n',
        )

    def test_logger_middleware(self):
        output = self.run_script('tests/fixtures/api_logger.abl')
        self.assertEqual(output, 'GET / 200\nok\n')


if __name__ == '__main__':
    unittest.main()
