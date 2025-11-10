# Hemlock Test Suite

This directory contains the test suite for the Hemlock interpreter.

## Running Tests

To run all tests:

```bash
./run_tests.sh
```

The test runner will:
- Build the project
- Run all tests organized by category
- Report results with colored output
- Show a summary at the end

## Test Organization

Tests are organized into categories:

- **arithmetic/** - Basic arithmetic operations, precedence, negation, division by zero
- **bools/** - Boolean literals and operators
- **comparisons/** - Equality and relational operators (==, !=, <, >, <=, >=)
- **control/** - Control flow (if, if-else, nested if, while)
- **conversions/** - Type conversions between different numeric types
- **pointers/** - Memory allocation, pointer arithmetic, memset, memcpy
- **primitives/** - Primitive type tests (i8, i16, i32, u8, u16, u32, f32, f64)
- **strings/** - String operations (concat, index, length, mutate, empty)
- **variables/** - Variable declaration and reassignment

## Test Naming Conventions

Tests with certain keywords in their names are expected to fail (error tests):
- **overflow** - Tests that values exceed type bounds
- **negative** - Tests with negative values where they're not allowed
- **invalid** - Tests with invalid syntax or semantics
- **error** - General error cases

These tests verify that the interpreter correctly rejects invalid code.

## Writing New Tests

To add a new test:

1. Create a `.hml` file in the appropriate category directory
2. For tests that should pass: use any descriptive name
3. For tests that should fail: include `overflow`, `negative`, `invalid`, or `error` in the filename
4. Run `./run_tests.sh` to verify your test

## Current Test Results

All 37 tests are passing:
- 33 regular tests pass
- 4 error tests correctly fail as expected

### Test Coverage

The test suite covers:
- ✅ All arithmetic operators (+, -, *, /)
- ✅ Operator precedence
- ✅ Unary negation
- ✅ All comparison operators (==, !=, <, >, <=, >=)
- ✅ Boolean operators (&&, ||, !)
- ✅ Control flow (if, if-else, nested if, while loops)
- ✅ Type conversions and promotions
- ✅ All primitive types (i8, i16, i32, u8, u16, u32, f32, f64)
- ✅ Range checking for typed variables
- ✅ String operations (concatenation, indexing, mutation, length)
- ✅ Memory management (alloc, free, memset, memcpy)
- ✅ Pointer arithmetic
- ✅ Variable reassignment
- ✅ Error handling (division by zero, overflow, underflow)
