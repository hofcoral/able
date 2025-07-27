#!/usr/bin/env python3
"""Build the Able interpreter and execute the test suite."""

import subprocess
import sys
import unittest
from typing import Iterable


class TrackingResult(unittest.TextTestResult):
    """Test result collecting successes for summary table."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.successes = []

    def addSuccess(self, test):
        super().addSuccess(test)
        self.successes.append(test)

    def addSubTest(self, test, subtest, outcome):
        super().addSubTest(test, subtest, outcome)
        if outcome is None:
            self.successes.append(f"{test} {subtest}")


class TableTestRunner(unittest.TextTestRunner):
    resultclass = TrackingResult

    def run(self, test):
        all_ids = list(self._iter_tests(test))
        result = super().run(test)
        self._print_table(all_ids, result)
        return result

    def _iter_tests(self, s: Iterable) -> Iterable[str]:
        for t in s:
            if isinstance(t, unittest.TestSuite):
                yield from self._iter_tests(t)
            else:
                yield str(t)

    def _print_table(self, tests: Iterable[str], result: TrackingResult):
        status = {tid: 'PASS' for tid in tests}
        for t, _ in result.failures:
            status[str(t)] = 'FAIL'
        for t, _ in result.errors:
            status[str(t)] = 'ERROR'
        for t, _ in result.skipped:
            status[str(t)] = 'SKIP'
        width = max(len(k) for k in status)
        print("\nTest Results:")
        print(f"{'Test'.ljust(width)} | Result")
        print('-' * (width + 9))
        for name in sorted(status):
            print(f"{name.ljust(width)} | {status[name]}")


def main():
    subprocess.run(["make"], check=True)

    suite = unittest.defaultTestLoader.discover("tests")
    runner = TableTestRunner(verbosity=2)
    result = runner.run(suite)
    if not result.wasSuccessful():
        print("Not all tests passed.")
        sys.exit(1)

    sys.exit(0)


if __name__ == "__main__":
    main()
