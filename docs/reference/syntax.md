# Syntax Notes

## Call and Attribute Chaining
Able currently supports attribute access and function calls only on identifier
chains. Method calls on literals or the result of another call are not parsed.

Unsupported:
- `1.to_string()`
- `a.plus(0.4).to_string()`
- `(make()).run()`

Workaround:
```
sum = a.plus(0.4)
pr(sum.to_string())
```
