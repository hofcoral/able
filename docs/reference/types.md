# Types and Numbers

Able exposes real type objects and a decimal `Number` type for precise math.

## Type Checks

Use the `is` operator to check types:

```able
x = 42
pr(x is Number)
pr("hi" is String)
pr([] is List)
```

`type(value)` returns the type object for a value. Use `type_name(value)` for a string name:

```able
pr(type(123) is Number)
pr(type_name(123))
```

## Number Basics

Number literals are decimal and all numeric operators use decimal arithmetic.
The default context is precision 20 with HALF_UP rounding.

```able
total = 0.1 + 0.2
pr(total.to_string())
```

## Number Methods

Numbers support safe decimal operations:

- `eq`, `lt`, `lte`, `gt`, `gte`, `cmp`
- `plus`, `minus`, `times`, `div`, `mod`, `pow`
- `abs`, `neg`, `sqrt`, `floor`, `ceil`, `round`
- `to_string`, `to_fixed`, `is_int`

```able
amount = 1.5
pr(amount.times(2).to_fixed(0))
pr(amount.is_int())
```

You can also construct numbers explicitly:

```able
pr(Number("12.5").plus(1).to_string())
```

## Value Ownership (Interpreter)

The interpreter uses a simple ownership model to avoid leaks:

- Most functions borrow values and clone when needed.
- Promise helpers have explicit ownership variants:
  - `promise_resolve` and `promise_reject` clone their arguments.
  - `promise_resolve_owned` and `promise_reject_owned` take ownership and do not clone.
