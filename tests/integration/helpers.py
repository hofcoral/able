import os
import subprocess
from pathlib import Path
import unittest

EXE = Path('build/able_exe')

class AbleTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        subprocess.run(['make'], check=True)
        if not EXE.exists():
            raise RuntimeError('Executable not built')

    def run_script(self, path: str) -> str:
        env = os.environ.copy()
        env.setdefault('ABLE_HTTP_FIXTURES', '1')
        result = subprocess.run([str(EXE), path], check=True, capture_output=True, text=True, env=env)
        return result.stdout
