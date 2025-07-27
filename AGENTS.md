# Able Agent Guidelines

## Overview
- **src/**: C implementation of the Able interpreter. Subfolders include `lexer`, `parser`, `ast`, `interpreter`, `types`, and `utils`.
- **examples/**: `.abl` files used for tests and documentation.
- **tests/**: Python integration tests exercising the interpreter using the scripts in `examples/`.
- **run_tests.py**: Convenience script that runs `make` and executes the test suite.
- **vendor/**: Third‑party headers. Do not modify these files.

## Contribution and Style Guidelines
- Keep the codebase modular and DRY. Each logical concern should live in its own module.
- Avoid hard‑coded values. Create reusable abstractions when possible.
- Maintain clear separation of concerns: parsing logic stays under `parser`, execution under `interpreter`, data structures under `types`, and so on.
- Follow the commit message style: `[type] Description`. Valid types include `feat`, `fix`, `patch`, `trivial`, etc. Use `[!feat]` for breaking changes. Group unrelated changes into separate commits.
- Favor small, focused pull requests with descriptive commit messages and PR bodies.

## Migration Work
- Portions of the interpreter are being gradually modularized. When migrating code, keep related functionality within its respective folder (e.g., lexer, parser, interpreter). Update tests and examples accordingly.

## Validating Changes
1. Run `python3 run_tests.py` from the repository root. This builds the interpreter with `make` and runs all tests under `tests/`.
2. Ensure the build and tests succeed before committing.

## Working Process
1. Explore the relevant source files under `src/` and associated tests in `tests/` when making changes.
2. Include documentation updates in `README.md` or `examples/` if behavior changes.
3. Format PR messages with a short title and a clear summary of what changed. Mention test results in a separate "Testing" section.

