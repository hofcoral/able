# Annotations in Able

Able supports two kinds of annotations on top-level assignments, functions, methods, and classes:

- **Modifiers** run after a target is created so they can mutate metadata (for example, tagging a function as `static`).
- **Decorators** behave like Python decorators—they can be called with arguments and return a replacement callable.

## Registering annotations

Able code can register handlers without touching the C runtime. Two builtins are available:

```abl
register_modifier(name: str, handler: fun(target, info))
register_decorator(name: str, factory: fun(...args) -> fun(target) -> target)
```

- `target` is the object being annotated.
- `info` is an object that exposes `target_type` (`"function"`, `"method"`, `"class"`, or `"assignment"`) and `name` when available.
- Modifier handlers can return an object with `assign_private: true` to mark the surrounding assignment as private.

Decorators always receive the target as their only argument. If an annotation was written with parentheses (e.g. `@label("greeted")`), the decorator handler is called first to obtain a callable and then invoked with the target.

## Example

The `examples/annotations/basic.abl` script demonstrates a modifier and decorator working together:

```abl
fun middleware_modifier(target, info):
    target.is_middleware = true

register_modifier("middleware", middleware_modifier)

fun label(prefix):
    fun decorate(fn):
        fun wrapper(name):
            return prefix + ": " + fn(name)
        return wrapper
    return decorate

register_decorator("label", label)

@middleware
@label("greeted")
fun greet(name):
    return "Hello " + name
```

Running `python3 run_tests.py` executes the integration test `tests/integration/test_annotations.py`, which covers this behavior.
