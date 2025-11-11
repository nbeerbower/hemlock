![Hemlock](logo.png)

# Hemlock

> A small, unsafe language for writing unsafe things safely.

Hemlock is a systems scripting language that combines the power of C with the ergonomics of modern scripting languages. It embraces manual memory management and explicit control while providing structured async concurrency built-in.

## Design Philosophy

- **Explicit over implicit** - No hidden behavior, no magic
- **Manual memory management** - You allocate, you free
- **Dynamic by default, typed by choice** - Optional type annotations with runtime checking
- **Unsafe is a feature** - Full control when you need it, safety tools when you want them

## Features

- **Familiar syntax** - C-like with modern improvements
- **Rich type system** - i8/i16/i32, u8/u16/u32, f32/f64, bool, string, ptr, buffer, null
- **First-class functions** - Closures, recursion, higher-order functions
- **Objects** - JavaScript-style objects with methods, duck typing, and JSON serialization
- **Two pointer types** - Raw `ptr` for experts, safe `buffer` with bounds checking
- **Memory API** - alloc, free, memset, memcpy, realloc, talloc, sizeof
- **Mutable strings** - First-class UTF-8 strings with indexing and concatenation
- **Structured concurrency** - async/await, spawn/join/detach (coming soon)

## Quick Examples

### Hello World
```hemlock
print("Hello, World!");
```

### Memory Management
```hemlock
// Raw pointer (dangerous but flexible)
let p: ptr = alloc(64);
memset(p, 0, 64);
free(p);

// Safe buffer (bounds checked)
let buf: buffer = buffer(64);
buf[0] = 65;
print(buf.length);  // 64
free(buf);
```

### Type System
```hemlock
let x = 42;              // i32 inferred
let y: u8 = 255;         // explicit u8
let z = x + y;           // promotes to i32
let pi: f64 = 3.14159;   // explicit precision
```

### String Operations
```hemlock
let s = "hello";
s[0] = 72;              // mutate to "Hello"
print(s.length);        // 5
let msg = s + " world"; // "Hello world"
```

### Functions and Closures
```hemlock
// Named function with recursion
fn factorial(n: i32): i32 {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

// Closures capture their environment
fn makeAdder(x) {
    return fn(y) {
        return x + y;
    };
}

let add5 = makeAdder(5);
print(add5(3));  // 8
```

### Objects
```hemlock
// Anonymous objects
let person = { name: "Alice", age: 30 };
print(person.name);

// Methods with self keyword
let counter = {
    count: 0,
    increment: fn() {
        self.count = self.count + 1;
    }
};
counter.increment();
print(counter.count);  // 1

// Type definitions with duck typing
define Person {
    name: string,
    age: i32,
    active?: true,  // Optional field with default
}

let p = { name: "Bob", age: 25 };
let typed_p: Person = p;  // Duck typing validates structure
print(typeof(typed_p));   // "Person"

// JSON serialization
let json = serialize(person);
print(json);  // {"name":"Alice","age":30}
let restored = deserialize(json);
```

## Building

```bash
make
```

## Running Tests

```bash
make test
```

## Running Programs

```bash
./hemlock program.hml
```

## Project Status

Hemlock is currently in early development (v0.1). The following features are implemented:

- ✅ Primitives and type system (i8-i32, u8-u32, f32/f64, bool, string, null)
- ✅ Memory management (ptr, buffer, alloc, free, talloc, realloc, memset, memcpy)
- ✅ String operations (mutable, indexing, concatenation, length)
- ✅ Control flow (if/else, while)
- ✅ Functions and closures (first-class, recursion, lexical scoping)
- ✅ Objects (literals, methods, duck typing, optional fields, JSON serialization)
- 🚧 Async/await and structured concurrency

## Why Hemlock?

Hemlock is for programmers who want:
- The power of C without the boilerplate
- Manual memory management without the verbosity
- Dynamic types with optional static guarantees
- Structured concurrency without the complexity

Hemlock is **NOT** memory-safe. Dangling pointers, use-after-free, and buffer overflows are your responsibility. We provide tools to help (`buffer`, type annotations, bounds checking) but we don't force you to use them.

## Philosophy on Safety

> "We give you the tools to be safe (`buffer`, type annotations, bounds checking) but we don't force you to use them (`ptr`, manual memory, unsafe operations). The default should guide toward safety, but the escape hatch should always be available."

## License

MIT License

## Contributing

Hemlock is experimental and evolving. If you're interested in contributing, please read `CLAUDE.md` first to understand the design philosophy.
