#!/usr/bin/env python3
"""Build the Able interpreter and execute the test suite."""

import subprocess
import sys
import unittest


def main():
    subprocess.run(["make"], check=True)

    suite = unittest.defaultTestLoader.discover("tests")
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if not result.wasSuccessful():
        print("Not all tests passed.")
        sys.exit(1)
    else:
        print("All tests passed.")
        sys.exit(0)


if __name__ == "__main__":
    main()
