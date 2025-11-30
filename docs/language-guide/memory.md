# Memory Management

Hemlock embraces **manual memory management** with explicit control over allocation and deallocation. This guide covers Hemlock's memory model, the two pointer types, and the complete memory API.

## Philosophy

Hemlock follows the principle: "You allocated it, you free it." There is:
- No garbage collection
- No automatic resource cleanup
- No reference counting (in v0.1)
- Full responsibility on the programmer

This explicit approach gives you complete control but requires careful management to avoid memory leaks and dangling pointers.

## The Two Pointer Types

Hemlock provides two distinct pointer types, each with different safety characteristics:

### `ptr` - Raw Pointer (Dangerous)

Raw pointers are **just addresses** with minimal safety guarantees:

```hemlock
let p: ptr = alloc(64);
memset(p, 0, 64);
free(p);  // You must remember to free
```

**Characteristics:**
- Just an 8-byte address
- No bounds checking
- No length tracking
- User manages lifetime entirely
- For experts and FFI

**Use cases:**
- Low-level system programming
- Foreign Function Interface (FFI)
- Performance-critical code
- When you need complete control

**Dangers:**
```hemlock
let p = alloc(10);
let q = p + 100;  // Way past allocation - allowed but dangerous
free(p);
let x = *p;       // Dangling pointer - undefined behavior
free(p);          // Double-free - will crash
```

### `buffer` - Safe Wrapper (Recommended)

Buffers provide **bounds-checked access** while still requiring manual deallocation:

```hemlock
let b: buffer = buffer(64);
b[0] = 65;              // bounds checked
print(b.length);        // 64
free(b);                // still manual
```

**Characteristics:**
- Pointer + length + capacity
- Bounds checked on access
- Still requires manual `free()`
- Better default for most code

**Properties:**
```hemlock
let buf = buffer(100);
print(buf.length);      // 100 (current size)
print(buf.capacity);    // 100 (allocated capacity)
```

**Bounds checking:**
```hemlock
let buf = buffer(10);
buf[5] = 42;      // OK
buf[100] = 42;    // ERROR: Index out of bounds
```

## Memory API

### Core Allocation

**`alloc(bytes)` - Allocate raw memory**
```hemlock
let p = alloc(1024);  // Allocate 1KB, returns ptr
// ... use memory
free(p);
```

**`buffer(size)` - Allocate safe buffer**
```hemlock
let buf = buffer(256);  // Allocate 256-byte buffer
buf[0] = 65;            // 'A'
buf[1] = 66;            // 'B'
free(buf);
```

**`free(ptr)` - Free memory**
```hemlock
let p = alloc(100);
free(p);  // Must free to avoid memory leak

let buf = buffer(100);
free(buf);  // Works on both ptr and buffer
```

**Important:** `free()` works on both `ptr` and `buffer` types.

### Memory Operations

**`memset(ptr, byte, size)` - Fill memory**
```hemlock
let p = alloc(100);
memset(p, 0, 100);     // Zero out 100 bytes
memset(p, 65, 10);     // Fill first 10 bytes with 'A'
free(p);
```

**`memcpy(dest, src, size)` - Copy memory**
```hemlock
let src = alloc(50);
let dst = alloc(50);
memset(src, 42, 50);
memcpy(dst, src, 50);  // Copy 50 bytes from src to dst
free(src);
free(dst);
```

**`realloc(ptr, size)` - Resize allocation**
```hemlock
let p = alloc(100);
// ... use 100 bytes
p = realloc(p, 200);   // Resize to 200 bytes
// ... use 200 bytes
free(p);
```

**Note:** After `realloc()`, the old pointer may be invalid. Always use the returned pointer.

### Typed Allocation (TODO - v0.2)

Future versions will include typed allocation helpers:

```hemlock
// Planned for v0.2
let arr = talloc(i32, 100);  // Allocate 100 i32 values
let size = sizeof(i32);      // Get size of type
```

## Common Patterns

### Pattern: Allocate, Use, Free

The basic pattern for memory management:

```hemlock
// 1. Allocate
let data = alloc(1024);

// 2. Use
memset(data, 0, 1024);
// ... do work

// 3. Free
free(data);
```

### Pattern: Safe Buffer Usage

Prefer buffers for bounds-checked access:

```hemlock
let buf = buffer(256);

// Safe iteration
let i = 0;
while (i < buf.length) {
    buf[i] = i;
    i = i + 1;
}

free(buf);
```

### Pattern: Resource Management with try/finally

Ensure cleanup even on errors:

```hemlock
let data = alloc(1024);
try {
    // ... risky operations
    process(data);
} finally {
    free(data);  // Always freed, even on error
}
```

## Memory Safety Considerations

### Double-Free

**Allowed but will crash:**
```hemlock
let p = alloc(100);
free(p);
free(p);  // CRASH: Double-free detected
```

**Prevention:**
```hemlock
let p = alloc(100);
free(p);
p = null;  // Set to null after freeing

if (p != null) {
    free(p);  // Won't execute
}
```

### Dangling Pointers

**Allowed but undefined behavior:**
```hemlock
let p = alloc(100);
*p = 42;      // OK
free(p);
let x = *p;   // UNDEFINED: Reading freed memory
```

**Prevention:** Don't access memory after freeing.

### Memory Leaks

**Easy to create, hard to debug:**
```hemlock
fn leak_memory() {
    let p = alloc(1000);
    // Forgot to free!
    return;  // Memory leaked
}
```

**Prevention:** Always pair `alloc()` with `free()`:
```hemlock
fn safe_function() {
    let p = alloc(1000);
    try {
        // ... use p
    } finally {
        free(p);  // Always freed
    }
}
```

### Pointer Arithmetic

**Allowed but dangerous:**
```hemlock
let p = alloc(10);
let q = p + 100;  // Way past allocation boundary
*q = 42;          // UNDEFINED: Out of bounds write
free(p);
```

**Use buffers for bounds checking:**
```hemlock
let buf = buffer(10);
buf[100] = 42;  // ERROR: Bounds check prevents overflow
```

## Best Practices

1. **Default to `buffer`** - Use `buffer` unless you specifically need raw `ptr`
2. **Match alloc/free** - Every `alloc()` should have exactly one `free()`
3. **Use try/finally** - Ensure cleanup with exception handling
4. **Null after free** - Set pointers to `null` after freeing to catch use-after-free
5. **Bounds check** - Use buffer indexing for automatic bounds checking
6. **Document ownership** - Make clear which code owns and frees each allocation

## Examples

### Example: Dynamic String Builder

```hemlock
fn build_message(count: i32): ptr {
    let size = count * 10;
    let buf = alloc(size);

    let i = 0;
    while (i < count) {
        memset(buf + (i * 10), 65 + i, 10);
        i = i + 1;
    }

    return buf;  // Caller must free
}

let msg = build_message(5);
// ... use msg
free(msg);
```

### Example: Safe Array Operations

```hemlock
fn process_array(size: i32) {
    let arr = buffer(size);

    try {
        // Fill array
        let i = 0;
        while (i < arr.length) {
            arr[i] = i * 2;
            i = i + 1;
        }

        // Process
        i = 0;
        while (i < arr.length) {
            print(arr[i]);
            i = i + 1;
        }
    } finally {
        free(arr);  // Always cleanup
    }
}
```

### Example: Memory Pool Pattern

```hemlock
// Simple memory pool (simplified)
let pool = alloc(10000);
let pool_offset = 0;

fn pool_alloc(size: i32): ptr {
    if (pool_offset + size > 10000) {
        throw "Pool exhausted";
    }

    let ptr = pool + pool_offset;
    pool_offset = pool_offset + size;
    return ptr;
}

// Use pool
let p1 = pool_alloc(100);
let p2 = pool_alloc(200);

// Free entire pool at once
free(pool);
```

## Limitations (v0.1)

Current limitations to be aware of:

- **No reference counting** - Objects and arrays are never freed automatically
- **No cycle detection** - Circular references will leak memory
- **No typed allocation** - `talloc()` and `sizeof()` planned for v0.2
- **No custom allocators** - Only system malloc/free

## Related Topics

- [Strings](strings.md) - String memory management and UTF-8 encoding
- [Arrays](arrays.md) - Dynamic arrays and their memory characteristics
- [Objects](objects.md) - Object allocation and lifetime
- [Error Handling](error-handling.md) - Using try/finally for cleanup

## See Also

- **Design Philosophy**: See CLAUDE.md section "Memory Management"
- **Type System**: See [Types](types.md) for `ptr` and `buffer` type details
- **FFI**: Raw pointers are essential for Foreign Function Interface
