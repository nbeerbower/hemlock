# Making Runtime Errors Catchable - Implementation Plan

## Overview

Currently, many runtime errors in Hemlock use `exit(1)` which makes them uncatchable by try-catch blocks. This document outlines the plan to make these errors catchable exceptions.

**Status:** Implementation in progress (helper function created, refactoring needed)

---

## Problem Statement

**Current Behavior:**
```hemlock
try {
    let ch = s.char_at(10);  // Index out of bounds
} catch (e) {
    print("Never caught!");  // This line never executes
}
// Program exits with: Runtime error: char_at() index 10 out of bounds
```

**Desired Behavior:**
```hemlock
try {
    let ch = s.char_at(10);  // Index out of bounds
} catch (e) {
    print("Caught: " + e);  // This should execute
}
```

---

## Root Cause

Errors currently use this pattern:
```c
if (index < 0 || index >= length) {
    fprintf(stderr, "Runtime error: index out of bounds\n");
    exit(1);  // ← Immediately exits, cannot be caught
}
```

**Solution:** Use exception mechanism instead:
```c
if (index < 0 || index >= length) {
    ctx->exception_state.exception_value = val_string("index out of bounds");
    value_retain(ctx->exception_state.exception_value);
    ctx->exception_state.is_throwing = 1;
    return val_null();
}
```

---

## Implementation Plan

### Phase 1: Create Helper Function ✅ DONE

Created `throw_runtime_error()` helper in `src/interpreter/builtins/internal_helpers.c`:

```c
static Value throw_runtime_error(ExecutionContext *ctx, const char *format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    ctx->exception_state.exception_value = val_string(buffer);
    value_retain(ctx->exception_state.exception_value);
    ctx->exception_state.is_throwing = 1;
    return val_null();
}
```

**Usage:**
```c
if (error_condition) {
    return throw_runtime_error(ctx, "Error: %s", reason);
}
```

---

### Phase 2: Thread ExecutionContext Through Method Signatures 🔄 IN PROGRESS

**Challenge:** Method handlers don't currently receive `ExecutionContext *ctx`

**Current Signatures:**
```c
Value call_string_method(String *str, const char *method, Value *args, int num_args);
Value call_array_method(Array *arr, const char *method, Value *args, int num_args);
Value call_file_method(FileHandle *file, const char *method, Value *args, int num_args);
Value call_channel_method(Channel *ch, const char *method, Value *args, int num_args);
```

**Needed Signatures:**
```c
Value call_string_method(String *str, const char *method, Value *args, int num_args, ExecutionContext *ctx);
Value call_array_method(Array *arr, const char *method, Value *args, int num_args, ExecutionContext *ctx);
Value call_file_method(FileHandle *file, const char *method, Value *args, int num_args, ExecutionContext *ctx);
Value call_channel_method(Channel *ch, const char *method, Value *args, int num_args, ExecutionContext *ctx);
```

**Files to Update:**
1. `src/interpreter/io/internal.h` - Update declarations
2. `src/interpreter/io/string_methods.c` - Update implementation + callers
3. `src/interpreter/io/array_methods.c` - Update implementation + callers
4. `src/interpreter/io/file_methods.c` - Update implementation + callers
5. `src/interpreter/io/channel_methods.c` - Update implementation + callers
6. `src/interpreter/runtime/expressions.c` - Update all method calls

---

### Phase 3: Convert Errors to Exceptions

**String Methods** (`src/interpreter/io/string_methods.c`):
- ✅ `char_at()` - Already uses clamping (no error)
- ✅ `byte_at()` - Already uses clamping (no error)
- ✅ `slice()` - Already uses clamping (no error)
- ✅ `substr()` - Already uses clamping (no error)
- ⚠️ `find()`, `contains()`, etc. - Check for exit(1) calls

**Array Methods** (`src/interpreter/io/array_methods.c`):
- ⚠️ `find()` - Check for errors
- ⚠️ `insert()` - Currently errors on out of bounds
- ⚠️ `remove()` - Currently errors on out of bounds
- ✅ `slice()` - Already uses clamping (no error)
- ⚠️ Other methods - Audit for exit(1)

**File Methods** (`src/interpreter/io/file_methods.c`):
- ⚠️ `read()` on closed file
- ⚠️ `write()` on closed file
- ⚠️ `seek()` on closed file
- ⚠️ All file operations - Convert to exceptions

**Channel Methods** (`src/interpreter/io/channel_methods.c`):
- ⚠️ `send()` on closed channel
- ⚠️ `recv()` on closed channel
- ⚠️ Close operations

**Builtin Functions** (various):
- ⚠️ Function arity mismatches
- ⚠️ Type validation errors
- ⚠️ alloc() with invalid size
- ⚠️ Memory operations

---

### Phase 4: Update Tests

**Currently Failing Tests (will pass once errors are catchable):**

1. `strings/edge_char_byte_at_bounds.hml`
2. `strings/edge_out_of_bounds.hml`
3. `arrays/edge_out_of_bounds.hml`
4. `async/edge_channel_closed.hml`
5. `async/edge_join_twice.hml`
6. `async/edge_detach_then_join.hml`
7. `functions/edge_arity_mismatch.hml`
8. `io/edge_closed_file_ops.hml`
9. `memory/edge_alloc_zero.hml`

**Expected Result:** All these tests should pass after conversion

---

## Detailed Conversion Steps

### Step-by-Step Process:

1. **Add throw_runtime_error() to each file that needs it:**
```c
// At top of file, after #include
static Value throw_runtime_error(ExecutionContext *ctx, const char *format, ...) {
    // ... (helper implementation)
}
```

2. **Update function signatures:**
```c
// Before:
Value call_string_method(String *str, const char *method, Value *args, int num_args) {

// After:
Value call_string_method(String *str, const char *method, Value *args, int num_args, ExecutionContext *ctx) {
```

3. **Update all callers in expressions.c:**
```c
// Before:
Value result = call_string_method(str, method, args, num_args);

// After:
Value result = call_string_method(str, method, args, num_args, ctx);
```

4. **Convert each error from exit() to exception:**
```c
// Before:
if (error_condition) {
    fprintf(stderr, "Runtime error: description\n");
    exit(1);
}

// After:
if (error_condition) {
    return throw_runtime_error(ctx, "description");
}
```

5. **Test each conversion:**
```bash
./hemlock tests/strings/edge_char_byte_at_bounds.hml
# Should print: "Caught char_at out of bounds: ..."
```

---

## Example Conversion

### Before:
```c
// string_methods.c
Value call_string_method(String *str, const char *method, Value *args, int num_args) {
    if (strcmp(method, "char_at") == 0) {
        if (index < 0 || index >= str->char_length) {
            fprintf(stderr, "Runtime error: char_at() index %d out of bounds (length=%d)\n",
                    index, str->char_length);
            exit(1);  // ← Uncatchable
        }
        // ...
    }
}
```

### After:
```c
// string_methods.c
static Value throw_runtime_error(ExecutionContext *ctx, const char *format, ...) {
    // ... helper implementation ...
}

Value call_string_method(String *str, const char *method, Value *args, int num_args, ExecutionContext *ctx) {
    if (strcmp(method, "char_at") == 0) {
        if (index < 0 || index >= str->char_length) {
            return throw_runtime_error(ctx, "char_at() index %d out of bounds (length=%d)",
                    index, str->char_length);  // ← Catchable!
        }
        // ...
    }
}
```

---

## Errors That Should REMAIN Fatal

Some errors should still use `exit(1)` because they indicate bugs in the interpreter itself:

### Fatal Errors (keep exit(1)):
- **Out of memory** - malloc/realloc failures
- **Invalid interpreter state** - Internal consistency checks
- **Stack overflow** - Call stack limit exceeded (maybe)
- **Assertion failures** - panic() and assert() should remain fatal

### Recoverable Errors (convert to exceptions):
- **Index out of bounds** - User can handle this
- **Type mismatches** - User can validate types
- **Invalid arguments** - User can check inputs
- **Resource errors** - File not found, channel closed, etc.
- **Arithmetic errors** - Division by zero (currently errors)

---

## Testing Plan

### Unit Tests:
```hemlock
// Test: Catch index out of bounds
try {
    let s = "hello";
    let ch = s.char_at(100);
    print("ERROR: Should have thrown");
} catch (e) {
    assert(e.contains("out of bounds"), "Should mention out of bounds");
    print("PASS: Caught error");
}

// Test: Catch channel error
try {
    let ch = channel(5);
    ch.close();
    ch.send(42);
    print("ERROR: Should have thrown");
} catch (e) {
    assert(e.contains("closed channel"), "Should mention closed channel");
    print("PASS: Caught error");
}
```

### Integration Tests:
- Run full test suite
- Verify all edge case tests pass
- Check that error messages are preserved
- Ensure stack traces work with exceptions

---

## Performance Considerations

### Exception Throwing Overhead:
- Creating exception value: ~100ns (string allocation)
- Setting exception state: ~10ns (struct fields)
- Unwinding stack: ~1μs per frame
- **Total:** Negligible for error cases (errors should be rare)

### Compared to exit(1):
- exit(1): Immediately terminates process
- Exception: Allows cleanup and recovery
- **Impact:** Exception is slightly slower but provides better UX

---

## Risks & Mitigation

### Risk 1: Breaking Changes
**Risk:** Changing error behavior might break existing code
**Mitigation:**
- This makes code MORE resilient (can catch errors)
- No existing code relies on uncatchable errors
- Only affects error cases (rare in correct programs)

### Risk 2: Incomplete Conversion
**Risk:** Missing some exit(1) calls during conversion
**Mitigation:**
- Grep for all exit(1) calls: `grep -r "exit(1)" src/`
- Run full test suite
- Add tests for each error type

### Risk 3: Memory Leaks
**Risk:** Exceptions might leak if not handled properly
**Mitigation:**
- Use value_retain() consistently
- Exception values are cleaned up by catch blocks
- Test with valgrind for leaks

---

## Timeline Estimate

### Small Scope (Bounds Checking Only):
- **Time:** 2-3 hours
- **Files:** 2-3 (string/array methods)
- **Tests Fixed:** 3-4

### Medium Scope (All Method Errors):
- **Time:** 4-6 hours
- **Files:** 5-7 (all method files)
- **Tests Fixed:** 7-10

### Full Scope (All Runtime Errors):
- **Time:** 8-12 hours
- **Files:** 10-15 (builtins + methods)
- **Tests Fixed:** 10-13

---

## Current Status

### Completed ✅:
- ✅ Created throw_runtime_error() helper function
- ✅ Identified all affected files
- ✅ Created implementation plan

### In Progress 🔄:
- 🔄 Threading ExecutionContext through signatures
- 🔄 Converting string/array methods

### Remaining ⏸️:
- ⏸️ File method conversions
- ⏸️ Channel method conversions
- ⏸️ Builtin function conversions
- ⏸️ Test updates and verification

---

## Recommendation

**Proceed with Medium Scope implementation:**
1. Complete ExecutionContext threading (1-2 hours)
2. Convert all method errors to exceptions (2-3 hours)
3. Test and verify (1 hour)
4. Document changes (30 min)

**Benefits:**
- Fixes 7-10 edge case tests
- Makes Hemlock more user-friendly
- Aligns with modern language design
- Enables better error handling patterns

**Alternative:** Keep current behavior and mark tests as expected-fatal-error
- Faster (30 minutes)
- Less risky
- But misses opportunity to improve language UX
