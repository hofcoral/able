# Able

Able is a small interpreted scripting language designed for scripting APIs and simple data manipulation.

## Build

Run:

```sh
make
```

This compiles the interpreter to `build/able_exe`.

## Running Able Files

Execute an `.abl` file with:

```sh
./build/able_exe path/to/script.abl
```

### Example

```able
set name to "Daniel"
pr(name)

# List usage
set numbers to [1, 2]
numbers.append(3)
pr(numbers.get(2))

# Integer loops
for i of 3:
    pr(i)

# Increment
set x to 0
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
    set init to (this, name):
        this.name to name
    set greet to (this):
        pr("Hello ", this.name)
set p to Person("Able")
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
set start to time()
sleep(1)
pr(time() - start >= 1)
```

Custom modules can also be loaded from the working directory:

```able
from examples.custom_utils import greet
pr(greet("Codex"))
```

Functions or variables marked with `@private` will not be exported when the module is imported.

## Development

- Source code lives in `src/`.
- Use `make clean` to remove build artifacts.

## License

MIT License.
