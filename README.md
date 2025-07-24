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
```

```
./build/able_exe examples/variables/basic_assignment.abl
```

## Development

- Source code lives in `src/`.
- Use `make clean` to remove build artifacts.

## License

MIT License.
