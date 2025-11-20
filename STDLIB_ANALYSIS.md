# Hemlock Standard Library Analysis & Update Proposals

**Date:** 2025-11-20
**Status:** Comprehensive analysis of current stdlib and proposed enhancements

---

## Current State Overview

### ✅ Implemented Modules

#### 1. **Math Module** (`@stdlib/math`)
**Status:** Fully implemented, well-designed
**Functions:** 21 math functions + 5 constants
- Trigonometry: sin, cos, tan, asin, acos, atan, atan2
- Exponential/Log: sqrt, pow, exp, log, log10, log2
- Rounding: floor, ceil, round, trunc
- Utility: abs, min, max, clamp
- Random: rand, rand_range, seed
- Constants: PI, E, TAU, INF, NAN

**Quality:** ✅ Comprehensive, maps directly to C stdlib

#### 2. **Time Module** (`@stdlib/time`)
**Status:** Basic implementation
**Functions:** 4 time functions
- now() - Unix timestamp (i64)
- time_ms() - Milliseconds since epoch (i64)
- clock() - CPU time in seconds (f64)
- sleep(seconds) - Delay execution

**Quality:** ✅ Functional but missing formatting/parsing

#### 3. **Environment Module** (`@stdlib/env`)
**Status:** Basic implementation
**Functions:** 5 environment/process functions
- getenv(name) - Get environment variable
- setenv(name, value) - Set environment variable
- unsetenv(name) - Unset environment variable
- exit(code?) - Exit with optional code
- get_pid() - Get process ID

**Quality:** ✅ Covers essentials

#### 4. **Filesystem Module** (`@stdlib/fs`)
**Status:** Comprehensive file operations
**Functions:** 16 filesystem functions
- File ops: exists, read_file, write_file, append_file, remove_file, rename, copy_file
- Directory ops: make_dir, remove_dir, list_dir
- File info: is_file, is_dir, file_stat
- Path ops: cwd, chdir, absolute_path

**Quality:** ✅ Very complete for basic file operations

#### 5. **Collections Module** (`@stdlib/collections`)
**Status:** Pure Hemlock implementations
**Data Structures:**
- HashMap - Hash table with O(1) operations
- Queue - FIFO queue
- Stack - LIFO stack
- Set - Unique values with set operations
- LinkedList - Doubly-linked list

**Quality:** ✅ Excellent implementations, well-tested

---

## 🔍 Gaps & Issues

### Documentation Issues

1. **CLAUDE.md Gaps:**
   - Stdlib modules (math, time, env, fs) are NOT documented
   - Only collections.hml mentioned in future plans
   - Users won't know these modules exist!
   - No import syntax examples for stdlib

2. **Outdated stdlib/README.md:**
   - Claims math, io, strings, os are "future modules" but they exist
   - Missing documentation for math, time, env, fs modules
   - Directory structure doesn't match reality

3. **No Public API Documentation:**
   - Modules exist but no comprehensive reference
   - No examples showing how to use stdlib modules

### Missing Critical Modules

#### 1. **String Utilities** (High Priority)
**Status:** ❌ Missing (only builtin string methods exist)

**Proposed Functions:**
```hemlock
// String building
fn join(parts: array, delimiter: string): string
fn pad_left(str: string, width: i32, fill?: string): string
fn pad_right(str: string, width: i32, fill?: string): string
fn center(str: string, width: i32, fill?: string): string

// Inspection
fn is_alpha(str: string): bool
fn is_digit(str: string): bool
fn is_alphanumeric(str: string): bool
fn is_whitespace(str: string): bool
fn is_upper(str: string): bool
fn is_lower(str: string): bool

// Advanced operations
fn reverse(str: string): string
fn count(str: string, substring: string): i32
fn index_of(str: string, substring: string, start?: i32): i32
fn last_index_of(str: string, substring: string): i32
fn lines(str: string): array<string>
fn words(str: string): array<string>
```

**Rationale:** String manipulation is fundamental, current builtin methods insufficient for real-world use.

#### 2. **Path Utilities** (High Priority)
**Status:** ❌ Missing (fs module has basic path ops but no manipulation)

**Proposed Functions:**
```hemlock
// Path manipulation
fn join(parts: array<string>): string      // Join path components
fn basename(path: string): string          // Get filename
fn dirname(path: string): string           // Get directory name
fn extname(path: string): string           // Get file extension
fn normalize(path: string): string         // Normalize path (resolve .., .)
fn relative(from: string, to: string): string  // Relative path
fn is_absolute(path: string): bool         // Check if absolute

// Path inspection
fn split(path: string): array<string>      // Split into components
fn parse(path: string): object             // {dir, base, ext, name}
```

**Rationale:** Path manipulation is essential for file operations, currently requires string manipulation.

#### 3. **JSON Module** (Medium Priority)
**Status:** ⚠️ Partially exists (object.serialize, string.deserialize) but undocumented

**Current State:**
- `obj.serialize()` exists - converts object to JSON string
- `json_str.deserialize()` exists - parses JSON string

**Proposed Improvements:**
```hemlock
// Formalize as module
export fn stringify(value: any, pretty?: bool): string
export fn parse(json: string): any
export fn validate(json: string): bool
export fn pretty_print(value: any, indent?: i32): string
```

**Rationale:** JSON support exists but hidden, needs proper module wrapper and documentation.

#### 4. **Encoding Module** (Medium Priority)
**Status:** ❌ Missing

**Proposed Functions:**
```hemlock
// Base64
fn base64_encode(data: buffer): string
fn base64_decode(str: string): buffer

// Hex
fn hex_encode(data: buffer): string
fn hex_decode(str: string): buffer

// URL encoding
fn url_encode(str: string): string
fn url_decode(str: string): string
```

**Rationale:** Common for data interchange, web APIs, binary data representation.

#### 5. **Testing Module** (Medium Priority)
**Status:** ❌ Missing (only `assert` builtin exists)

**Proposed Functions:**
```hemlock
// Test framework
fn test(name: string, fn: function): null
fn describe(name: string, fn: function): null
fn expect(value: any): Assertion

// Assertion object
define Assertion {
    to_equal(expected: any): null
    to_be_null(): null
    to_be_true(): null
    to_be_false(): null
    to_throw(message?: string): null
    to_be_greater_than(n: any): null
    to_be_less_than(n: any): null
    to_contain(item: any): null
}
```

**Rationale:** Testing is critical for library development, current `assert()` is too basic.

#### 6. **HTTP Module** (Low Priority - Complex)
**Status:** ❌ Missing

**Proposed API:**
```hemlock
// Simple HTTP client
fn get(url: string, headers?: object): Response
fn post(url: string, body: string, headers?: object): Response
fn put(url: string, body: string, headers?: object): Response
fn delete(url: string, headers?: object): Response

define Response {
    status: i32
    headers: object
    body: string
}
```

**Rationale:** Essential for modern applications, but requires libcurl or similar (dependency).

#### 7. **Regex Module** (Low Priority - Complex)
**Status:** ❌ Missing

**Proposed API:**
```hemlock
// Regular expressions
fn match(pattern: string, text: string): bool
fn match_all(pattern: string, text: string): array
fn replace(pattern: string, text: string, replacement: string): string
fn split(pattern: string, text: string): array<string>

// Compiled regex
fn compile(pattern: string): Regex
define Regex {
    test(text: string): bool
    exec(text: string): object
}
```

**Rationale:** Advanced text processing, but requires PCRE or similar (dependency).

---

## 📊 Priority Matrix

### Phase 1: Documentation & Cleanup (Immediate)
**Effort:** Low | **Impact:** High

1. ✅ **Update CLAUDE.md** - Document existing stdlib modules
2. ✅ **Update stdlib/README.md** - Accurate module listing
3. ✅ **Create stdlib/docs/** - Individual module docs (math.md, time.md, env.md, fs.md)
4. ✅ **Add usage examples** - Show import syntax in CLAUDE.md

### Phase 2: Essential Missing Modules (Short-term)
**Effort:** Medium | **Impact:** High

1. ✅ **strings.hml** - String utilities (pure Hemlock, no C needed)
2. ✅ **path.hml** - Path manipulation (pure Hemlock)
3. ✅ **json.hml** - Formalize existing JSON support

### Phase 3: Advanced Utilities (Medium-term)
**Effort:** Medium | **Impact:** Medium

1. ⚠️ **encoding.hml** - Base64, hex, URL encoding (may need C for base64)
2. ⚠️ **testing.hml** - Test framework (pure Hemlock)
3. ⚠️ **datetime.hml** - Date/time formatting (extend time.hml)

### Phase 4: External Dependencies (Long-term)
**Effort:** High | **Impact:** Variable

1. ❌ **http.hml** - HTTP client (requires libcurl)
2. ❌ **regex.hml** - Regular expressions (requires PCRE)
3. ❌ **crypto.hml** - Cryptographic functions (requires OpenSSL)
4. ❌ **compression.hml** - zlib/gzip (requires zlib)

---

## 🎯 Recommended Actions

### Immediate (Next PR)

**1. Fix Documentation Gaps**
- Add stdlib section to CLAUDE.md
- Document all existing modules (math, time, env, fs, collections)
- Add import syntax examples
- Update stdlib/README.md to match reality

**2. Create Module Documentation**
```
stdlib/docs/
├── math.md       # Full API reference
├── time.md       # Time functions
├── env.md        # Environment variables
├── fs.md         # Filesystem operations
└── collections.md # Already exists
```

### Short-term (Next 2-3 PRs)

**3. Add strings.hml Module**
```hemlock
// Pure Hemlock implementation
export fn join(parts, delimiter) { ... }
export fn pad_left(str, width, fill?) { ... }
export fn is_alpha(str) { ... }
// ... etc
```

**4. Add path.hml Module**
```hemlock
// Pure Hemlock path manipulation
export fn join(parts) { ... }
export fn basename(path) { ... }
export fn dirname(path) { ... }
// ... etc
```

**5. Add json.hml Module**
```hemlock
// Wrapper around existing serialize/deserialize
export fn stringify(value, pretty?) {
    if (typeof(value) == "object") {
        return value.serialize();
    }
    // ... handle primitives
}

export fn parse(json) {
    return json.deserialize();
}
```

### Medium-term (Future PRs)

**6. Add encoding.hml Module** (may need C builtins for base64)
**7. Add testing.hml Module** (pure Hemlock test runner)
**8. Enhance time.hml** with formatting functions

### Long-term (v0.2+)

**9. HTTP Module** - Requires libcurl integration via FFI
**10. Regex Module** - Requires PCRE integration via FFI
**11. Crypto Module** - Requires OpenSSL integration via FFI

---

## 📝 Notes on Implementation Strategy

### Pure Hemlock vs C Builtins

**Pure Hemlock Preferred For:**
- String utilities (can use existing string methods)
- Path manipulation (string operations)
- JSON wrapper (already have serialize/deserialize)
- Collections (already done)
- Testing framework (pure logic)

**C Builtins Required For:**
- Base64 encoding (complex bit manipulation)
- Compression (external library)
- Cryptography (security-critical, use OpenSSL)
- HTTP (complex networking, use libcurl)
- Regex (complex parsing, use PCRE)

### FFI Strategy

For modules requiring external libraries, Hemlock's FFI support enables:
```hemlock
// Use FFI to call C libraries directly
import { dlopen, dlsym } from "@stdlib/ffi";

let lib = dlopen("libcurl.so.4");
let curl_easy_init = dlsym(lib, "curl_easy_init");
// ... build wrapper
```

This allows gradual addition of complex modules without modifying core.

---

## 🔄 Comparison with Other Languages

### Python Standard Library Coverage

| Module | Python | Hemlock | Status |
|--------|--------|---------|--------|
| Math | ✅ | ✅ | Complete |
| Time | ✅ | ⚠️ | Basic only |
| File I/O | ✅ | ✅ | Complete |
| Path | ✅ | ❌ | Missing |
| JSON | ✅ | ⚠️ | Exists but undocumented |
| Regex | ✅ | ❌ | Missing |
| HTTP | ✅ | ❌ | Missing |
| Collections | ✅ | ✅ | Excellent |
| String utils | ✅ | ⚠️ | Only methods, no module |
| Base64 | ✅ | ❌ | Missing |
| Testing | ✅ | ❌ | Only assert() |

### Rust Standard Library Coverage

| Module | Rust | Hemlock | Status |
|--------|------|---------|--------|
| Collections | ✅ | ✅ | Good |
| File I/O | ✅ | ✅ | Complete |
| Path | ✅ | ❌ | Missing |
| Env | ✅ | ✅ | Basic |
| Time | ✅ | ⚠️ | No formatting |
| Process | ✅ | ⚠️ | Only exec() |
| Net | ✅ | ❌ | Missing |

**Conclusion:** Hemlock has ~60% of typical stdlib coverage, focusing on essentials. Priority should be:
1. Documentation (unlock existing features)
2. String/Path utilities (pure Hemlock, low-hanging fruit)
3. JSON formalization (already implemented)
4. Advanced modules via FFI (long-term)

---

## Summary

**Current Strengths:**
- ✅ Math module is comprehensive
- ✅ Collections are excellent (pure Hemlock)
- ✅ File I/O is complete
- ✅ Environment/process basics covered

**Critical Gaps:**
- ❌ Documentation - modules exist but undocumented
- ❌ String utilities - only methods, no module
- ❌ Path manipulation - essential for file work
- ❌ JSON - exists but hidden

**Recommended Focus:**
1. **Fix documentation** (unlock 40% more value instantly)
2. **Add strings.hml** (high-value, pure Hemlock)
3. **Add path.hml** (high-value, pure Hemlock)
4. **Formalize json.hml** (already implemented)
5. **Long-term:** HTTP, regex via FFI

**Philosophy Alignment:**
- Keep core minimal ✅
- Provide essential tools ✅
- No magic, explicit ✅
- Allow unsafe when needed ✅
- Focus on systems scripting ✅
