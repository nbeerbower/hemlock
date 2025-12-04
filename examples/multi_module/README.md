# Multi-Module Example

This example demonstrates Hemlock's module system with multiple local imports.

## Structure

```
multi_module/
├── main.hml          # Entry point - imports from both modules
├── math_utils.hml    # Math functions: square, cube, factorial, etc.
└── string_utils.hml  # String functions: reverse, repeat, palindrome check
```

## Running

```bash
# Run directly (uses module system)
./hemlock examples/multi_module/main.hml

# Bundle into single file
./hemlock --bundle examples/multi_module/main.hml -o multi.hmlc

# Run bundled version
./hemlock multi.hmlc

# Bundle with compression
./hemlock --bundle examples/multi_module/main.hml --compress -o multi.hmlb
```

## Output

```
=== Multi-Module Example ===

--- Math Utils ---
PI = 3.14159
square(5) = 25
cube(3) = 27
circle_area(2) = 12.5664
factorial(5) = 120
is_even(42) = true

--- String Utils ---
repeat_string('ha', 3) = hahaha
reverse_string('hello') = olleh
is_palindrome('radar') = true
is_palindrome('hello') = false

=== Example Complete ===
```
