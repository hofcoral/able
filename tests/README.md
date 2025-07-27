The test suite compiles the interpreter and runs Able scripts found in the
`examples/` directory. Test modules live under `tests/integration` and use a
shared helper for invoking the interpreter.

Run `python3 run_tests.py` from the repository root to build the executable and
execute all tests. The runner prints a summary table showing the result of each
test case.
