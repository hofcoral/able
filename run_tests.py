#!/usr/bin/env python3
"""Build the Able interpreter and execute the test suite."""

import subprocess
import sys
import tempfile
import os
import unittest


def main():
    subprocess.run(["make"], check=True)

    suite = unittest.defaultTestLoader.discover("tests")
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if not result.wasSuccessful():
        print("Not all tests passed.")
        sys.exit(1)

    def run(code):
        with tempfile.NamedTemporaryFile("w", suffix=".abl", delete=False) as f:
            f.write(code)
            temp_path = f.name
        try:
            out = subprocess.run(["build/able_exe", temp_path], check=True, capture_output=True, text=True)
            return out.stdout
        finally:
            os.unlink(temp_path)

    def run_raises(code):
        try:
            run(code)
        except subprocess.CalledProcessError:
            return True
        return False

    assert run('set x to "foo"\npr(type(x))') == 'STRING\n'
    assert run('set x to 123\npr(type(x))') == 'NUMBER\n'
    assert run('set x to true\npr(type(x))') == 'BOOLEAN\n'
    assert run('set x to null\npr(type(x))') == 'NULL\n'
    assert run('set x to {}\npr(type(x))') == 'OBJECT\n'
    assert run('set f to (): return\npr(type(f))') == 'FUNCTION\n'
    assert run_raises('pr(type())')
    assert run_raises('pr(type(a,b))')

    print("All tests passed.")
    sys.exit(0)


if __name__ == "__main__":
    main()
