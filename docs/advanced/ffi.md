# Foreign Function Interface (FFI) in Hemlock

Hemlock provides **FFI (Foreign Function Interface)** to call C functions from shared libraries using libffi, enabling integration with existing C libraries and system APIs.

## Table of Contents

- [Overview](#overview)
- [Current Status](#current-status)
- [Supported Types](#supported-types)
- [Basic Concepts](#basic-concepts)
- [Use Cases](#use-cases)
- [Future Development](#future-development)

## Overview

The Foreign Function Interface (FFI) allows Hemlock programs to:
- Call C functions from shared libraries (.so, .dylib, .dll)
- Use existing C libraries without writing wrapper code
- Access system APIs directly
- Integrate with third-party native libraries
- Bridge Hemlock with low-level system functionality

**Key capabilities:**
- Dynamic library loading
- C function binding
- Automatic type conversion between Hemlock and C types
- Support for all primitive types
- libffi-based implementation for portability

## Current Status

FFI support is available in Hemlock v0.1 with the following features:

**Implemented:**
- ✅ Call C functions from shared libraries
- ✅ Support for all primitive types (integers, floats, pointers)
- ✅ Automatic type conversion
- ✅ libffi-based implementation
- ✅ Dynamic library loading

**In Development:**
- 🔄 Struct passing and return values
- 🔄 Array/buffer handling
- 🔄 Function pointer callbacks
- 🔄 String marshaling helpers
- 🔄 Error handling improvements

**Test Coverage:**
- 3 FFI tests passing (as of v0.1)
- Basic function calling verified
- Type conversion tested

## Supported Types

### Primitive Types

The following Hemlock types can be passed to/from C functions:

| Hemlock Type | C Type | Size | Notes |
|--------------|--------|------|-------|
| `i8` | `int8_t` | 1 byte | Signed 8-bit integer |
| `i16` | `int16_t` | 2 bytes | Signed 16-bit integer |
| `i32` | `int32_t` | 4 bytes | Signed 32-bit integer |
| `i64` | `int64_t` | 8 bytes | Signed 64-bit integer |
| `u8` | `uint8_t` | 1 byte | Unsigned 8-bit integer |
| `u16` | `uint16_t` | 2 bytes | Unsigned 16-bit integer |
| `u32` | `uint32_t` | 4 bytes | Unsigned 32-bit integer |
| `u64` | `uint64_t` | 8 bytes | Unsigned 64-bit integer |
| `f32` | `float` | 4 bytes | 32-bit floating point |
| `f64` | `double` | 8 bytes | 64-bit floating point |
| `ptr` | `void*` | 8 bytes | Raw pointer |

### Type Conversion

**Automatic conversions:**
- Hemlock integers → C integers (with range checking)
- Hemlock floats → C floats
- Hemlock pointers → C pointers
- C return values → Hemlock values

**Example type mappings:**
```hemlock
// Hemlock → C
let i: i32 = 42;         // → int32_t (4 bytes)
let f: f64 = 3.14;       // → double (8 bytes)
let p: ptr = alloc(64);  // → void* (8 bytes)

// C → Hemlock (return values)
// int32_t foo() → i32
// double bar() → f64
// void* baz() → ptr
```

## Basic Concepts

### Shared Libraries

FFI works with compiled shared libraries:

**Linux:** `.so` files
```
libexample.so
/usr/lib/libm.so
```

**macOS:** `.dylib` files
```
libexample.dylib
/usr/lib/libSystem.dylib
```

**Windows:** `.dll` files
```
example.dll
kernel32.dll
```

### Function Signatures

C functions must have known signatures for FFI to work correctly:

```c
// Example C function signatures
int add(int a, int b);
double sqrt(double x);
void* malloc(size_t size);
void free(void* ptr);
```

These can be called from Hemlock once the library is loaded and functions are bound.

### Platform Compatibility

FFI uses **libffi** for portability:
- Works on x86, x86-64, ARM, ARM64
- Handles calling conventions automatically
- Abstracts platform-specific ABI details
- Supports Linux, macOS, Windows (with appropriate libffi)

## Use Cases

### 1. System Libraries

Access standard C library functions:

**Math functions:**
```hemlock
// Call sqrt from libm
let result = sqrt(16.0);  // 4.0
```

**Memory allocation:**
```hemlock
// Call malloc/free from libc
let ptr = malloc(1024);
free(ptr);
```

### 2. Third-Party Libraries

Use existing C libraries:

**Example: Image processing**
```hemlock
// Load libpng or libjpeg
// Process images using C library functions
```

**Example: Cryptography**
```hemlock
// Use OpenSSL or libsodium
// Encryption/decryption via FFI
```

### 3. System APIs

Direct system calls:

**Example: POSIX APIs**
```hemlock
// Call getpid, getuid, etc.
// Access low-level system functionality
```

### 4. Performance-Critical Code

Call optimized C implementations:

```hemlock
// Use highly-optimized C libraries
// SIMD operations, vectorized code
// Hardware-accelerated functions
```

### 5. Hardware Access

Interface with hardware libraries:

```hemlock
// GPIO control on embedded systems
// USB device communication
// Serial port access
```

### 6. Legacy Code Integration

Reuse existing C codebases:

```hemlock
// Call functions from legacy C applications
// Gradually migrate to Hemlock
// Preserve working C code
```

## Future Development

### Planned Features

**1. Struct Support**
```hemlock
// Future: Pass/return C structs
define Point {
    x: f64,
    y: f64,
}

let p = Point { x: 1.0, y: 2.0 };
c_function_with_struct(p);
```

**2. Array/Buffer Handling**
```hemlock
// Future: Better array passing
let arr = [1, 2, 3, 4, 5];
process_array(arr);  // Pass to C function
```

**3. Function Pointer Callbacks**
```hemlock
// Future: Pass Hemlock functions to C
fn my_callback(x: i32): i32 {
    return x * 2;
}

c_function_with_callback(my_callback);
```

**4. String Marshaling**
```hemlock
// Future: Automatic string conversion
let s = "hello";
c_string_function(s);  // Auto-convert to C string
```

**5. Error Handling**
```hemlock
// Future: Better error reporting
try {
    let result = risky_c_function();
} catch (e) {
    print("FFI error: " + e);
}
```

**6. Type Safety**
```hemlock
// Future: Type annotations for FFI
@ffi("libm.so")
fn sqrt(x: f64): f64;

let result = sqrt(16.0);  // Type-checked
```

### Roadmap

**v0.1 (Current):**
- ✅ Basic FFI with primitive types
- ✅ Dynamic library loading
- ✅ Function calling

**v0.2 (Planned):**
- 🔄 Struct support
- 🔄 Array handling
- 🔄 String marshaling
- 🔄 Function pointers

**v0.3 (Future):**
- 🔄 Advanced type mapping
- 🔄 Automatic binding generation
- 🔄 FFI debugging tools
- 🔄 Performance optimizations

## Current Limitations

As of v0.1, FFI has the following limitations:

**1. No Struct Passing**
- Cannot pass complex structs by value
- Workaround: Pass pointers to structs

**2. Manual Type Conversion**
- Must manually manage string conversions
- No automatic Hemlock string ↔ C string conversion

**3. Limited Error Handling**
- Basic error reporting
- No detailed FFI error messages yet

**4. No Callback Support**
- Cannot pass Hemlock functions to C
- C functions with callbacks not yet supported

**5. Manual Library Loading**
- Must manually load libraries
- No automatic binding generation

**6. Platform-Specific Code**
- Library paths differ by platform
- Must handle .so vs .dylib vs .dll

## Best Practices

While comprehensive FFI documentation is still being developed, here are general best practices:

### 1. Type Safety

```hemlock
// Be explicit about types
let x: i32 = 42;
let result: f64 = c_function(x);
```

### 2. Memory Management

```hemlock
// Remember to free allocated memory
let ptr = c_malloc(1024);
// ... use ptr
c_free(ptr);
```

### 3. Error Checking

```hemlock
// Check return values
let result = c_function();
if (result == null) {
    print("C function failed");
}
```

### 4. Platform Compatibility

```hemlock
// Handle platform differences
// Use appropriate library extensions (.so, .dylib, .dll)
```

## Examples

Detailed examples will be added as FFI documentation is expanded. For now, refer to:
- Test suite: `/tests/ffi/` (3 tests)
- Example programs: `/examples/` (if available)

## Getting Help

FFI is a newer feature in Hemlock. For questions or issues:

1. Check test suite for working examples
2. Refer to libffi documentation for low-level details
3. Report bugs or request features via project issues

## Summary

Hemlock's FFI provides:

- ✅ C function calling from shared libraries
- ✅ Primitive type support (i8-i64, u8-u64, f32, f64, ptr)
- ✅ Automatic type conversion
- ✅ libffi-based portability
- ✅ Foundation for native library integration

**Current status:** Basic FFI available in v0.1 with primitive types

**Coming soon:** Structs, arrays, callbacks, string marshaling

**Use cases:** System libraries, third-party libraries, performance-critical code, hardware access

---

**Note:** This documentation reflects FFI features as of Hemlock v0.1. FFI is under active development, and this document will be updated as features are added. For the most current information, refer to the test suite and CLAUDE.md.

## Contributing

FFI documentation is being expanded. If you're working with FFI:
- Document your use cases
- Share example code
- Report issues or limitations
- Suggest improvements

The FFI system is designed to be practical and safe while providing low-level access when needed, following Hemlock's philosophy of "explicit over implicit" and "unsafe is a feature, not a bug."
