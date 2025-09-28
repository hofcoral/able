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

#### Example: Power (`**`) Operator
```diff
// src/lexer/lexer.h
 typedef enum
 {
     TOKEN_STAR,
+    TOKEN_POW,
     TOKEN_SLASH,
```

```diff
// src/ast/ast.h
 typedef enum
 {
     OP_ADD,
     OP_SUB,
     OP_MUL,
     OP_DIV,
     OP_MOD,
+    OP_POW,
```

```c
// src/parser/parser.c (excerpt)
static ASTNode *parse_power()
{
    ASTNode *node = parse_unary();
    if (current.type == TOKEN_POW)
    {
        int op_line = current.line;
        int op_col = current.column;
        advance_token();
        ASTNode *rhs = parse_power(); // Right-associative
        ASTNode *bin = new_node(NODE_BINARY, op_line, op_col);
        bin->data.binary.op = OP_POW;
        add_child(bin, node);
        add_child(bin, rhs);
        return bin;
    }
    return node;
}

static ASTNode *parse_factor()
{
    ASTNode *node = parse_power();
    while (current.type == TOKEN_STAR || current.type == TOKEN_SLASH || current.type == TOKEN_PERCENT)
    {
        /* existing multiplicative parsing */
```

```diff
// src/interpreter/interpreter.c
        if (left.type == VAL_NUMBER && right.type == VAL_NUMBER)
        {
            Value res = {.type = VAL_NUMBER};
            switch (n->data.binary.op)
            {
            case OP_ADD:
                res.num = left.num + right.num;
                break;
            case OP_SUB:
                res.num = left.num - right.num;
                break;
            case OP_MUL:
                res.num = left.num * right.num;
                break;
            case OP_DIV:
                res.num = right.num != 0 ? left.num / right.num : 0;
                break;
            case OP_MOD:
                res.num = fmod(left.num, right.num);
                break;
+            case OP_POW:
+                res.num = pow(left.num, right.num);
+                break;
```

```abl
# examples/power.abl
fun square(x):
    return x ** 2

pr(square(8))
```

```python
# tests/integration/test_power.py
from tests.helpers import AbleTestCase


class PowerOperatorTests(AbleTestCase):
    def test_power_operator(self):
        result = self.run_script("examples/power.abl")
        self.assertEqual(result.stdout.strip(), "64")
```

### Introducing a Builtin Function
1. Extend the builtin registration so the global environment exposes the name.
2. Implement the helper either as native C code or inside `lib/builtins/`.
3. Document it with an example script.
4. Add tests verifying expected output.

#### Example: `clamp` Convenience Helper
```diff
// src/interpreter/builtins.c
 void builtins_register(Env *global_env, const char *file_path)
 {
-    const char *funcs[] = {"pr", "input", "type", "len", "bool", "int", "float",
-                            "str", "list", "dict", "range"};
+    const char *funcs[] = {"pr",   "input", "type",  "len",  "bool",  "int",
+                           "float", "str",   "list", "dict", "range", "clamp"};
     Value undef = {.type = VAL_UNDEFINED};
```

```diff
# lib/builtins/__init__.abl
-from "builtins/math" import abs, min, max
+from "builtins/math" import abs, min, max
+from "builtins/numeric" import clamp
```

```abl
# lib/builtins/numeric.abl
fun clamp(value, lo, hi):
    if value < lo:
        return lo
    if value > hi:
        return hi
    return value
```

```abl
# examples/builtins_clamp.abl
from "builtins" import clamp

pr(clamp(128, 0, 100))
```

```python
# tests/integration/test_builtins.py
class BuiltinClampTests(AbleTestCase):
    def test_clamp(self):
        result = self.run_script("examples/builtins_clamp.abl")
        self.assertEqual(result.stdout.strip(), "100")
```

### Creating a New Module
1. Implement module glue inside `src/interpreter/module.c` if new loading
   behavior is required.
2. Place Able source under `lib/<module>` (using `__init__.abl` for packages).
3. Add `.abl` examples illustrating import and usage patterns.
4. Add integration tests that import the module and assert results.

#### Example: Shipping a `json` Module
```abl
# lib/json/__init__.abl
from "json/parser" import parse

fun dumps(obj):
    if type(obj) == "object":
        parts = []
        for key of obj:
            parts.append("\"" + str(key) + "\":" + dumps(obj[key]))
        return "{" + ",".join(parts) + "}"
    if type(obj) == "list":
        return "[" + ",".join([dumps(x) for x of obj]) + "]"
    if type(obj) == "string":
        return "\"" + obj + "\""
    return str(obj)
```

```abl
# lib/json/parser.abl
fun parse_number(tokenizer):
    value = tokenizer.consume_number()
    return float(value)

fun parse(text):
    tokenizer = JsonTokenizer(text)
    return tokenizer.parse_value()
```

```diff
// examples/json_roundtrip.abl
+from "json" import dumps, parse
+
+text = "{"a": 1, "b": [2, 3]}"
+obj = parse(text)
+pr(dumps(obj))
```

```python
# tests/integration/test_json_module.py
class JsonModuleTests(AbleTestCase):
    def test_roundtrip(self):
        result = self.run_script("examples/json_roundtrip.abl")
        self.assertIn('{"a": 1, "b": [2, 3]}', result.stdout)
```

### Extending Runtime Types
1. Add or update type definitions in `src/types`.
2. Register new behaviors in `type_registry.c`.
3. Ensure the interpreter understands how to operate on the new type (e.g.,
   arithmetic, iteration).
4. Cover behavior with integration tests.

#### Example: Introducing a `range` Type
```diff
// src/types/value.h
 struct Object; // Forward declaration (to avoid circular include)
 struct Function; // Forward declaration for functions
 struct List;    // Forward declaration for lists
 struct Type;
 struct Instance;
+struct Range;
 
 typedef enum
 {
     VAL_UNDEFINED,
     VAL_NULL,
     VAL_BOOL,
     VAL_NUMBER,
     VAL_STRING,
     VAL_OBJECT,
     VAL_FUNCTION,
     VAL_LIST,
     VAL_TYPE,
     VAL_INSTANCE,
     VAL_BOUND_METHOD,
+    VAL_RANGE,
     VAL_TYPE_COUNT
 } ValueType;
 
 typedef struct Value
 {
     ValueType type;
     union
     {
         bool boolean;
         double num;
         char *str;
         struct Object *obj;
         struct Function *func;
         struct List *list;
         struct Type *cls;
         struct Instance *instance;
         BoundMethod *bound;
+        struct Range *range;
     };
 } Value;
```

```c
// src/types/range.h
#ifndef RANGE_H
#define RANGE_H

typedef struct Range
{
    double cursor;
    double stop;
    double step;
} Range;

Range *range_create(double start, double stop, double step);
void range_free(Range *range);
bool range_next(Range *range, double *out);

#endif
```

```c
// src/types/range.c
#include <stdlib.h>
#include "types/range.h"

Range *range_create(double start, double stop, double step)
{
    Range *range = malloc(sizeof(Range));
    range->cursor = start;
    range->stop = stop;
    range->step = step;
    return range;
}

void range_free(Range *range)
{
    free(range);
}

bool range_next(Range *range, double *out)
{
    if ((range->step > 0 && range->cursor >= range->stop) ||
        (range->step < 0 && range->cursor <= range->stop))
        return false;

    *out = range->cursor;
    range->cursor += range->step;
    return true;
}
```

```diff
// src/types/value.c
+#include "types/range.h"
@@
     case VAL_OBJECT:
         free_object(v.obj);
         break;
     case VAL_LIST:
         free_list(v.list);
         break;
+    case VAL_RANGE:
+        range_free(v.range);
+        break;
@@
     case VAL_OBJECT:
         copy.obj = clone_object(src->obj);
         break;
     case VAL_FUNCTION:
         copy.func = src->func;
         break;
     case VAL_LIST:
         copy.list = clone_list(src->list);
         break;
+    case VAL_RANGE:
+        copy.range = range_create(src->range->cursor, src->range->stop, src->range->step);
+        break;
```

```diff
// src/types/type_registry.c
     register_type(type_create("object"));
     register_type(type_create("function"));
     register_type(type_create("list"));
+    register_type(type_create("range"));
 }
```

```diff
// src/interpreter/interpreter.c (iteration support)
     case NODE_FOR:
     {
         Value iterable = eval_node(n->children[0]);
         ASTNode *body = n->children[1];
+        if (iterable.type == VAL_RANGE)
+        {
+            Range *range = iterable.range;
+            double current;
+            while (range_next(range, &current))
+            {
+                Value next = {.type = VAL_NUMBER, .num = current};
+                set_variable(interpreter_current_env(), n->data.loop.loop_var, next);
+                last = run_ast(body->children, body->child_count);
+                CallFrame *cf = current_frame(&call_stack);
+                if (cf && cf->returning)
+                    break;
+                if (break_flag)
+                {
+                    break_flag = false;
+                    break;
+                }
+                if (continue_flag)
+                {
+                    continue_flag = false;
+                    continue;
+                }
+            }
+            break;
+        }
         if (iterable.type == VAL_NUMBER)
         {
             /* existing loop */
```

```abl
# examples/range_iteration.abl
from "builtins" import range

for value of range(0, 5, 2):
    pr(value)
```

```python
# tests/integration/test_range_type.py
class RangeTypeTests(AbleTestCase):
    def test_range_iteration(self):
        result = self.run_script("examples/range_iteration.abl")
        self.assertEqual(result.stdout.splitlines(), ["0", "2", "4"])
```

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
