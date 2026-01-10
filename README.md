# Able

Able is a small interpreted scripting language designed for scripting APIs and simple data manipulation.

## Build

Run:

```sh
make
```

This compiles the interpreter to `build/able_exe`.

## Running Able Files

Execute an Able file with the `.abl` extension:

```sh
./build/able_exe path/to/script.abl
```

### Example

```able
name = "Daniel"
pr(name)

# List usage
numbers = [1, 2]
numbers.append(3)
pr(numbers.get(2))

# Integer loops
for i of 3:
    pr(i)

# Increment
x = 0
x++
pr(x)
```
```able
# Logical operators
pr(true and false)
pr(true or false)
pr(not false)
```
```able
# Class usage
class Person():
    fun init(this, name):
        this.name = name
    fun greet(this):
        pr("Hello ", this.name)
p = Person("Able")
p.greet()
```

```
./build/able_exe examples/oop/class.abl
```


```
./build/able_exe examples/variables/basic_assignment.abl
```

## Standard Library

Able ships with a small standard library. Some functions are available
globally, such as `abs`, `min` and `max`. Additional modules can be imported
using `import`:

```able
from math import sqrt
pr(sqrt(9))
```

Rounding helpers are also available:

```able
from math import ceil, round
pr(ceil(23 / 10))
pr(round(26 / 10))
```

The `time` module offers simple timing utilities:

```able
from time import time, sleep
start = time()
sleep(1)
pr(time() - start >= 1)
```

String and path helpers are available too:

```able
from string import trim, split, join
from path import normalize

pr(trim("  Able  "))
pr(join(split("a,b,c", ","), "-"))
pr(normalize("/api"))
```

## Language Notes

- Assignments inside functions are local to that function and do not mutate outer scope variables.
- `null` prints as `null`; `undefined` prints as `undefined`.
- `type(value)` returns a type object (use `type_name(value)` for a string).
- Numbers use decimal arithmetic with precision 20 and HALF_UP rounding.

```able
x = 5

fun bump(x):
    x = x + 1
    return x

pr(bump(1))
pr(x)
```

```able
amount = 0.1 + 0.2
pr(amount.to_string())
pr(amount is Number)
```

## Documentation

- `docs/README.md` - entry point for reference material and guides.
- `docs/reference/README.md` - language reference topics.
- `docs/MAINTENANCE.md` - interpreter architecture and contributor workflow.

Custom modules can also be loaded from the working directory:

```able
from examples.custom_utils import greet
pr(greet("Codex"))
```

Functions or variables marked with `@private` will not be exported when the module is imported.

## API Router

Able ships with a tiny annotation-driven router helper in `api`. It converts
annotation metadata into the `{ method, path, handler }` records expected by
`server_listen`:

```able
from api import router, Route, Get, Post

ROUTER = router()

@Route("/api")
class UserController():
    @Get
    fun index(this, request):
        return "ok"

    @Post("/users")
    fun create(this, request):
        return "created"

@Get("/health")
fun health(request):
    return "ok"

ROUTER.register(health)

server_listen({
    host: "127.0.0.1",
    port: 8080,
    routes: ROUTER.build()
})
```

`ROUTER.build()` also populates `ROUTER.errors` with any invalid route metadata.

Handlers can return plain values (string/number/bool) for text responses, or
objects/lists to send JSON. When returning a response object, `body` may also be
an object/list and will be JSON serialized automatically.

To enable request logging, use the built-in middleware:

```able
from api import logger
ROUTER.use(logger)
```

## Development

- Source code lives in `src/`.
- Use `make clean` to remove build artifacts.

## License

MIT License.
