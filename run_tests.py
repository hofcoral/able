#!/usr/bin/env python3
import subprocess
import sys
import unittest


def main():
    subprocess.run(['make'], check=True)
    suite = unittest.defaultTestLoader.discover('tests')
    result = unittest.TextTestRunner().run(suite)
    if not result.wasSuccessful():
        sys.exit(1)

if __name__ == '__main__':
    main()
