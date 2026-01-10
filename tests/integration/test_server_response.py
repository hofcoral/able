import http.client
import os
import socket
import subprocess
import time
import unittest
from pathlib import Path

from tests.integration.helpers import AbleTestCase, EXE


class ServerResponseTests(AbleTestCase):
    def _find_free_port(self) -> int:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.bind(("127.0.0.1", 0))
            return sock.getsockname()[1]

    def _request_with_retry(self, port: int, attempts: int = 40):
        last_error = None
        for _ in range(attempts):
            try:
                conn = http.client.HTTPConnection("127.0.0.1", port, timeout=1)
                conn.request("GET", "/")
                resp = conn.getresponse()
                body = resp.read().decode()
                headers = dict(resp.getheaders())
                status = resp.status
                conn.close()
                return status, headers, body
            except (OSError, http.client.HTTPException) as exc:
                last_error = exc
                time.sleep(0.05)
        raise last_error

    def _stop_process(self, proc: subprocess.Popen) -> tuple[str, str]:
        proc.terminate()
        try:
            stdout, stderr = proc.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            stdout, stderr = proc.communicate(timeout=5)
        return stdout, stderr

    def test_json_body_and_logger(self):
        port = self._find_free_port()
        port_path = Path("build/server_port.txt")
        port_path.write_text(str(port), encoding="utf-8")

        env = os.environ.copy()
        env.setdefault("ABLE_HTTP_FIXTURES", "1")
        proc = subprocess.Popen(
            [str(EXE), "tests/fixtures/server_response.abl"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            env=env,
        )

        try:
            status, headers, body = self._request_with_retry(port)
            self.assertEqual(status, 200)
            self.assertEqual(headers.get("Content-Type"), "application/json; charset=utf-8")
            self.assertEqual(body, '{"ok":true}')
        finally:
            stdout, stderr = self._stop_process(proc)
            if port_path.exists():
                port_path.unlink()

        self.assertNotIn("[ERROR", stderr)


if __name__ == "__main__":
    unittest.main()
