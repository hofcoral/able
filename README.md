# Able

**Able** is a small interpreted scripting language designed for scripting APIs and simple data manipulation.

## 🔧 Build

To build the project, run:

```sh
make
```

This will produce an executable in `bin/able`.
You can then run Able scripts with the `able` helper script, which builds
the interpreter if needed.

## Running Able Files

To run an `.abl` file, use:

```sh
./able path/to/yourfile.abl
```

Replace `path/to/yourfile.abl` with the path to your Able source file.

### Example

Suppose you have a file called `examples/example.abl`:

```able
set x to "hello"
print(x)
```

Run it with:

```sh
./able examples/example.abl
```

## Notes

- Make sure your `.abl` files use the correct syntax as described in the documentation.
- If you encounter errors, check your file for syntax issues or missing dependencies.
- The interpreter prints errors to stderr and exits on parse errors.

## Development

- Source code is in the `src/` directory.
- To clean build artifacts, run `make clean`.

## License

MIT License.
