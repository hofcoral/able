# Able Maintenance Guide

## Purpose and Audience
This guide is written for engineers responsible for evolving Able's interpreter and
standard library. It summarizes the architecture, development workflow, and
extension patterns so that you can contribute confidently without regressing
existing behavior.

---

## High-Level Architecture
Able is a single binary C interpreter built from modular subsystems located in
`src/`. The primary directories and their roles are:

- **`src/main.c`** – Entry point. Orchestrates the build pipeline: read source,
  tokenize, parse, build the runtime environment, run the AST, then tear
  everything down.
- **`src/lexer/`** – Converts raw source into tokens. `lexer.c` implements the
  scanner and error reporting; `lexer.h` exposes the token stream API used by the
  parser.
- **`src/parser/`** – Consumes tokens into an AST. `parser.c` contains Pratt
  parser machinery and statement parsing routines; `parser.h` exports
  `parse_program` and supporting interfaces.
- **`src/ast/`** – Defines the AST node graph, factories, and memory management.
  This layer isolates parser output from interpreter execution.
- **`src/types/`** – Runtime objects. `value.c` models primitive values, `object.c`
  and `type.c` define common object/type behaviors, `instance.c` and
  `list.c` provide container implementations, `env.c` manages lexical scope, and
  `type_registry.c` wires runtime types together.
- **`src/interpreter/`** – Executes Able code. `interpreter.c` drives evaluation,
  `stack.c` maintains the call stack, `resolve.c` handles identifier lookup,
  `attr.c` resolves attribute access, `module.c` implements import semantics, and
  `builtins.c` registers core functions and standard library modules.
- **`src/utils/`** – Shared helpers: file I/O, diagnostics, memory utilities, and
  convenience wrappers used throughout the interpreter.
- **`vendor/`** – Third-party headers. Treat as read-only.

Supporting assets include:

- **`examples/`** – Canonical `.abl` scripts that double as fixtures for tests and
  documentation.
- **`tests/`** – Python integration tests that compile the interpreter, execute
  the example scripts, and assert on their output (`run_tests.py` orchestrates
  the workflow).
- **`Makefile`** – Builds the interpreter (`build/able_exe`), compiles sources,
  and provides `clean` and `run` helpers.

---

## Build and Test Workflow
1. **Build** – Run `make` to compile `build/able_exe`. The makefile already
   encapsulates compiler flags, include paths, and dependency ordering.
2. **Run a script** – Execute `./build/able_exe path/to/script.abl`.
3. **Test** – Execute `python3 run_tests.py`. This rebuilds the interpreter and
   runs all Python integration tests in `tests/integration`, printing a summary
   table with pass/fail results.
4. **Clean** – `make clean` removes build artifacts when you need to force a full
   rebuild.

Automation expectations:

- Every change must keep the test suite green.
- Add regression coverage in `tests/integration` whenever you change semantics.
- Prefer extending existing examples or adding new scripts under `examples/` to
  exercise new features.

---

## Coding Standards and Project Conventions
- **Modularity** – Respect directory boundaries. Lexer logic stays under
  `src/lexer`, parser logic stays under `src/parser`, runtime data structures
  stay under `src/types`, and interpreter execution lives under
  `src/interpreter`.
- **Separation of concerns** – Keep parsing, evaluation, and type management
  isolated. Shared helpers belong in `src/utils`.
- **Memory management** – Every allocation must have a well-defined owner. The
  interpreter tears down runtime state with `free_ast`, `env_release`, and other
  cleanup helpers. Mirror that pattern when adding allocations.
- **Error handling** – Propagate lexer/parser failures up to the caller. Use the
  logging utilities in `src/utils/utils.c` for user-facing diagnostics.
- **Style** – Follow existing C99 conventions (`-Wall -Wextra -std=c99`). Keep
  functions short and cohesive. Use descriptive names and avoid hardcoded magic
  values—introduce enums or constants where applicable.
- **Threading** – The interpreter is single-threaded. Avoid shared global state
  unless it is properly encapsulated (e.g., the global type registry).

---

## Repository Layout at a Glance
```
.
├── docs/MAINTENANCE.md      # This guide
├── examples/                # Reference Able programs
├── src/                     # Interpreter implementation
│   ├── ast/                 # AST declarations and helpers
│   ├── interpreter/         # Runtime execution engine
│   ├── lexer/               # Tokenization
│   ├── parser/              # Parsing routines
│   ├── types/               # Runtime types and environment
│   └── utils/               # Shared utilities
├── tests/                   # Python integration suite
├── run_tests.py             # Build + test harness
└── Makefile                 # Build orchestration
```

---

## Deep Dives by Subsystem
### Lexer (`src/lexer`)
- **Key structs**: `Lexer`, `Token`.
- **Responsibilities**: Manage scanning state, produce tokens, flag syntax
  errors early. Central entry points include `lexer_init` and `lexer_next`.
- **Extending**: To add a new token type, update the token enum, extend scanning
  logic, and ensure the parser understands the new symbol.

### Parser and AST (`src/parser`, `src/ast`)
- **Parser**: Implements expression precedence and statement parsing via a Pratt
  parser. All control flow constructs originate here.
- **AST**: Defines node tags (e.g., literals, function declarations, loops) and
  provides constructors/destructors.
- **Extending**: When introducing a new syntax form, update the parser to build a
  new AST node and add the node type plus memory management hooks in `src/ast`.

### Runtime Types (`src/types`)
- **Core abstractions**: `Value` (boxed representation of runtime data),
  `Object` (base struct for heap entities), `Type` (runtime type descriptor),
  `Instance` (user-defined classes), `List`, and `Env` (lexical scope frames).
- **Type registration**: `type_registry.c` wires builtin types into the global
  registry; `type_registry.h` exposes lookup helpers.
- **Extending**: To add a new builtin type, create a `type_*.c` that defines the
  type methods, register it with the registry, and expose conversion helpers. For
  collections, ensure you update garbage-collection-style cleanup to release any
  nested objects.

### Interpreter (`src/interpreter`)
- **`interpreter.c`**: Walks the AST and evaluates nodes.
- **`stack.c`**: Manages call frames, argument passing, and returns.
- **`resolve.c`**: Looks up identifiers across scopes using `Env` frames.
- **`attr.c`**: Handles attribute and method access on runtime objects.
- **`module.c`**: Implements Able's module loader (`import`/`from` statements),
  handling search paths and module caching.
- **`builtins.c`**: Registers core functions and module exports into the global
  environment during startup.
- **Extending**: Add new interpreter behaviors by expanding the AST visitor in
  `interpreter.c`. Keep evaluation logic pure—stateful helpers belong in
  specialized files (e.g., attribute handling in `attr.c`).

### Utilities (`src/utils`)
- **`utils.c`** centralizes cross-cutting helpers: logging (`log_info`,
  `log_error`), file I/O (`read_file`), and defensive macros. Reuse them instead
  of duplicating functionality.

### Tests (`tests/integration`)
- **Structure**: Python `unittest` modules import `helpers.AbleTestCase` to build
  the interpreter and execute scripts from `examples/`.
- **Adding coverage**: Create a new `test_*.py` next to existing modules and
  leverage `run_script` to capture interpreter output. Each test should reference
  an `.abl` example to keep fixtures version-controlled.

---

## Standard Library and Modules
- Builtins load from `src/interpreter/builtins.c` and register into the global
  environment during startup.
- Module loading uses `module_system_init` and `module_system_cleanup` to manage
  search paths and caching.
- Time-sensitive utilities (e.g., `time`, `sleep`) live in the `time` module; math
  helpers ship with the interpreter.
- To ship a new standard module, implement its runtime functions in C, register
  them inside `builtins.c`, and optionally provide example scripts demonstrating
  usage.

---

## Contribution Workflow
1. **Plan** – Identify the subsystem you need to modify and confirm whether an
   existing example covers it.
2. **Isolate changes** – Follow atomic commits with messages like
   `[feat] Support XYZ literal`. Avoid bundling unrelated refactors.
3. **Implement** – Extend the relevant module while keeping responsibilities
   separated. Factor reusable logic into helper functions.
4. **Document** – Update `README.md`, `docs/`, or inline comments when behavior
   changes. Add or update `.abl` samples under `examples/`.
5. **Test** – Run `python3 run_tests.py` and ensure everything passes.
6. **Code review** – Provide a clear PR summary and list of tests executed.

---

## Common Extension Scenarios
### Adding a New Expression Type
1. Update token definitions in the lexer if a new symbol is required.
2. Teach the parser to produce a new AST node.
3. Implement evaluation logic inside `interpreter.c`.
4. Add regression tests in `tests/integration` with supporting scripts in
   `examples/`.

### Introducing a Builtin Function
1. Define the function in `src/interpreter/builtins.c`.
2. Register it within `builtins_register` so it appears in the global scope.
3. Document it with an example script.
4. Add tests verifying expected output.

### Creating a New Module
1. Implement module glue inside `src/interpreter/module.c` if new loading
   behavior is required.
2. Expose public APIs via `builtins_register` or by preloading the module.
3. Add `.abl` examples illustrating import and usage patterns.
4. Add integration tests that import the module and assert results.

### Extending Runtime Types
1. Add or update type definitions in `src/types`.
2. Register new behaviors in `type_registry.c`.
3. Ensure the interpreter understands how to operate on the new type (e.g.,
   arithmetic, iteration).
4. Cover behavior with integration tests.

---

## Operational Tips
- Use `make run file=examples/...` to quickly exercise a script while iterating.
- When debugging, sprinkle `log_debug` (or add a temporary variant) to trace
  execution; remember to remove or guard noisy logging before merging.
- Watch for memory leaks—`valgrind` is helpful when available.
- Keep the interpreter deterministic. Avoid reliance on undefined behavior or
  platform-specific C extensions.

---

## Release Hygiene
- Ensure `make clean && make` succeeds from a fresh checkout.
- Verify `python3 run_tests.py` passes on the target platform.
- Bump any version metadata (if added later) and update documentation to reflect
  new language features.
- Tag releases only after the interpreter is stable and tests are green.

---

## Getting Help
- Review existing tests and examples to understand expected semantics.
- Consult `README.md` for quick-start instructions.
- Explore prior commits for patterns when modifying core systems like the
  interpreter or type registry.

Happy hacking, and keep Able lean, predictable, and well-tested.
