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
- **Error handling** - try/catch/finally/throw for exception-based error handling
- **Command-line arguments** - Access program arguments via built-in `args` array
- **Structured concurrency** - async/await syntax, real OS threads via pthreads, thread-safe channels

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

### Command-Line Arguments
```hemlock
// Access arguments via the built-in 'args' array
// args[0] is the script name
// args[1+] are the actual arguments

print("Script: " + args[0]);
print("Total arguments: " + typeof(args.length));

if (args.length > 1) {
    let i = 1;
    while (i < args.length) {
        print(args[i]);
        i = i + 1;
    }
}
```

Run with:
```bash
./hemlock script.hml arg1 arg2 "argument with spaces"
```

### Error Handling
```hemlock
// Basic try/catch
try {
    throw "something went wrong";
} catch (e) {
    print("Error: " + e);
}

// Try/finally (cleanup always runs)
try {
    print("Attempting operation...");
} finally {
    print("Cleanup");
}

// Try/catch/finally
fn divide(a, b) {
    if (b == 0) {
        throw "division by zero";
    }
    return a / b;
}

try {
    let result = divide(10, 0);
    print(result);
} catch (e) {
    print("Caught: " + e);
} finally {
    print("Done");
}

// Any value can be thrown
try {
    throw { code: 404, message: "Not found" };
} catch (e) {
    print(serialize(e));  // {"code":404,"message":"Not found"}
}

// Re-throwing
try {
    try {
        throw "inner error";
    } catch (e) {
        print("Logging: " + e);
        throw e;  // Re-throw to outer handler
    }
} catch (e) {
    print("Outer: " + e);
}
```

### Async/Concurrency (TRUE Multi-Threading!)
```hemlock
// Async functions for concurrent execution
async fn compute(n: i32): i32 {
    let sum = 0;
    let i = 0;
    while (i < n) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}

// Spawn tasks on separate OS threads (real pthreads!)
let t1 = spawn(compute, 1000);
let t2 = spawn(compute, 2000);
let t3 = spawn(compute, 3000);

// Wait for results (all three computing in parallel!)
let r1 = join(t1);
let r2 = join(t2);
let r3 = join(t3);

print("Results: " + typeof(r1) + ", " + typeof(r2) + ", " + typeof(r3));
```

**Producer-Consumer with Channels:**
```hemlock
async fn producer(ch, count: i32) {
    let i = 0;
    while (i < count) {
        ch.send(i * 10);
        i = i + 1;
    }
    ch.close();
    return null;
}

async fn consumer(ch): i32 {
    let sum = 0;
    let val = ch.recv();
    while (val != null) {
        sum = sum + val;
        val = ch.recv();
    }
    return sum;
}

let ch = channel(10);
let p = spawn(producer, ch, 5);
let c = spawn(consumer, ch);

join(p);
let total = join(c);  // 100
print("Total: " + typeof(total));
```

**What makes this special:**
- ✅ Real OS threads (POSIX pthreads) - not green threads or coroutines
- ✅ True parallelism across multiple CPU cores
- ✅ Thread-safe channels with blocking send/recv
- ✅ Exception propagation across thread boundaries
- ✅ Same threading model as C/C++/Rust

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
# Run a program
./hemlock program.hml

# Run with command-line arguments
./hemlock program.hml arg1 arg2 arg3

# Start REPL
./hemlock
```

## Project Status

Hemlock is currently in early development (v0.1). The following features are implemented:

- ✅ Primitives and type system (i8-i32, u8-u32, f32/f64, bool, string, null)
- ✅ Memory management (ptr, buffer, alloc, free, talloc, realloc, memset, memcpy)
- ✅ String operations (mutable, indexing, concatenation, length)
- ✅ Control flow (if/else, while, for, break, continue)
- ✅ Functions and closures (first-class, recursion, lexical scoping)
- ✅ Objects (literals, methods, duck typing, optional fields, JSON serialization)
- ✅ Arrays (dynamic arrays with push/pop operations)
- ✅ Error handling (try/catch/finally/throw)
- ✅ File I/O (open, read, write, close, seek, tell)
- ✅ Command-line arguments (built-in `args` array)
- ✅ Async/await and structured concurrency (real OS threads via pthreads, channels, spawn/join/detach)

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
