# HTTP and WebSocket Test Suite

Comprehensive test coverage for the `@stdlib/http` and `@stdlib/websocket` modules.

## Summary

**Total test files created:** 5

### HTTP Module Tests (2 files)
- ✅ `tests/stdlib_http/test_http_basic.hml` - No dependencies
- ⚠️ `tests/stdlib_http/test_http_requests.hml` - Requires libwebsockets + network

### WebSocket Module Tests (3 files)
- ⚠️ `tests/stdlib_websocket/test_websocket_basic.hml` - Requires libwebsockets
- ⚠️ `tests/stdlib_websocket/test_websocket_echo.hml` - Requires libwebsockets + network
- ⚠️ `tests/stdlib_websocket/test_websocket_server.hml` - Requires libwebsockets

## Test Coverage

### HTTP Module (`@stdlib/http`)

| Feature | Basic Test | Request Test |
|---------|------------|--------------|
| Module loading | ✓ | ✓ |
| is_success() | ✓ | ✓ |
| is_redirect() | ✓ | - |
| is_client_error() | ✓ | - |
| is_server_error() | ✓ | - |
| url_encode() | ✓ | - |
| get() | - | ✓ |
| post() | - | ✓ |
| fetch() | - | ✓ |
| post_json() | - | ✓ |
| HTTPS (SSL/TLS) | - | ✓ |

**Total: 11 features tested**

### WebSocket Module (`@stdlib/websocket`)

| Feature | Basic | Echo | Server |
|---------|-------|------|--------|
| Module loading | ✓ | ✓ | ✓ |
| is_secure_url() | ✓ | - | - |
| parse_ws_url() | ✓ | - | - |
| WebSocket client | - | ✓ | ✓ |
| send_text() | - | ✓ | ✓ |
| recv() | - | ✓ | ✓ |
| close() | - | ✓ | ✓ |
| WebSocketServer | - | - | ✓ |
| accept() | - | - | ✓ |
| Bidirectional messaging | - | - | ✓ |
| Async/concurrency | - | - | ✓ |

**Total: 11 features tested**

## Running Tests

### Prerequisites

**For HTTP tests:**
```bash
# Install libwebsockets
sudo apt-get install libwebsockets-dev  # Ubuntu/Debian
brew install libwebsockets              # macOS

# Build FFI wrapper
make stdlib
```

**For WebSocket tests:**
- Same as HTTP (libwebsockets-dev + make stdlib)

### Run All Tests

```bash
# Via test runner (will include HTTP/WebSocket tests)
make test

# Or run individually
./hemlock tests/stdlib_http/test_http_basic.hml
./hemlock tests/stdlib_http/test_http_requests.hml
./hemlock tests/stdlib_websocket/test_websocket_basic.hml
./hemlock tests/stdlib_websocket/test_websocket_echo.hml
./hemlock tests/stdlib_websocket/test_websocket_server.hml
```

### Quick Test (No Dependencies)

```bash
# This test requires NO external libraries or network
./hemlock tests/stdlib_http/test_http_basic.hml
```

Expected output:
```
Testing @stdlib/http basic functionality...
✓ HTTP module loads
✓ Status code helpers work correctly
✓ URL encoding works

All HTTP basic tests passed!
```

## Test Structure

All tests follow a consistent pattern:

```hemlock
// 1. Header with requirements
// Requires: libwebsockets-dev, network connectivity
// Run with: ./hemlock tests/...

// 2. Import module
import { ... } from "@stdlib/module";

// 3. Test counters
let tests_passed = 0;
let tests_failed = 0;

// 4. Individual tests with try/catch
print("Test 1: Feature X");
try {
    // Test code
    assert(condition, "Description");
    tests_passed = tests_passed + 1;
} catch (e) {
    print("✗ Test failed: " + e);
    tests_failed = tests_failed + 1;
}

// 5. Summary
print("========================================");
print("Test Summary:");
print("  Passed: " + tests_passed);
print("  Failed: " + tests_failed);
print("========================================");

// 6. Final assertion
assert(tests_failed == 0, "All tests should pass");
```

## Documentation

Each test directory includes a comprehensive README:

- **tests/stdlib_http/README.md** - HTTP testing guide
  - Installation instructions
  - Test descriptions
  - Troubleshooting
  - Common issues

- **tests/stdlib_websocket/README.md** - WebSocket testing guide
  - Installation instructions
  - Test descriptions
  - Troubleshooting
  - Common issues

- **TESTING.md** - General testing guide
  - Test organization
  - Writing tests
  - Performance testing
  - Contributing tests

## Cleanup Performed

The following temporary test files were removed:
- ❌ `test_http_simple.hml` (deleted)
- ❌ `test_http_debug.hml` (deleted)
- ❌ `test_websocket_ffi.hml` (deleted)

All tests are now organized in proper test directories.

## Expected Test Results

### With libwebsockets installed:

```bash
$ make stdlib
Building libwebsockets wrapper...
gcc -shared -fPIC -o stdlib/c/lws_wrapper.so ...
✓ Build successful

$ ./hemlock tests/stdlib_http/test_http_basic.hml
Testing @stdlib/http basic functionality...
✓ HTTP module loads
✓ Status code helpers work correctly
✓ URL encoding works
All HTTP basic tests passed!

$ ./hemlock tests/stdlib_http/test_http_requests.hml
Testing HTTP requests with libwebsockets...
✓ GET request successful (status: 200)
✓ POST request successful (status: 200)
✓ fetch() returned 300 bytes
✓ POST JSON successful
✓ HTTPS request successful
========================================
HTTP Request Tests Summary:
  Passed: 5
  Failed: 0
========================================
```

### Without libwebsockets:

```bash
$ make stdlib
Building libwebsockets wrapper...
fatal error: libwebsockets.h: No such file or directory
make: *** [stdlib] Error 1

$ ./hemlock tests/stdlib_http/test_http_basic.hml
# Still works! No FFI needed for basic tests
Testing @stdlib/http basic functionality...
✓ HTTP module loads
✓ Status code helpers work correctly
✓ URL encoding works
All HTTP basic tests passed!

$ ./hemlock tests/stdlib_http/test_http_requests.hml
# Fails gracefully
✗ Failed to load module: No library imported before extern declaration
```

## Integration with Test Runner

The test runner (`tests/run_tests.sh`) automatically discovers and runs all `.hml` files in `tests/`, including:

- `tests/stdlib_http/*.hml`
- `tests/stdlib_websocket/*.hml`

Tests are categorized by directory:
```
[stdlib_http]
✓ stdlib_http/test_http_basic.hml
✗ stdlib_http/test_http_requests.hml (requires libwebsockets)

[stdlib_websocket]
✗ stdlib_websocket/test_websocket_basic.hml (requires libwebsockets)
✗ stdlib_websocket/test_websocket_echo.hml (requires libwebsockets)
✗ stdlib_websocket/test_websocket_server.hml (requires libwebsockets)
```

## Troubleshooting

### "No library imported before extern declaration"

**Cause:** libwebsockets-dev not installed or lws_wrapper.so not compiled

**Solution:**
```bash
sudo apt-get install libwebsockets-dev
make stdlib
```

### "libwebsockets.so: cannot open shared object file"

**Cause:** libwebsockets runtime library not found

**Solution:**
```bash
sudo apt-get install libwebsockets
# or
sudo ldconfig
```

### Network tests fail/timeout

**Cause:** No internet connectivity or test endpoints down

**Solution:**
- Check network: `ping httpbin.org`
- Check firewall rules
- Use different test endpoints (modify test files)

### Port already in use (WebSocket server test)

**Cause:** Port 9001 in use by another process

**Solution:**
```bash
# Kill process using port
lsof -ti:9001 | xargs kill

# Or change port in test file
let server = WebSocketServer("127.0.0.1", 9002);
```

## Next Steps

### Potential Improvements

1. **Mock HTTP server** - For reliable testing without network
2. **Performance benchmarks** - Measure request/response times
3. **Stress tests** - Test with large payloads and many connections
4. **Error injection** - Test error handling paths
5. **CI/CD integration** - Automated testing on push

### Extending Tests

To add new tests:
1. Create file in appropriate directory
2. Follow the test structure pattern
3. Add to README if introducing new features
4. Run full test suite to ensure no regressions

## Files Created/Modified

**New test files:**
- `tests/stdlib_http/test_http_basic.hml` (new)
- `tests/stdlib_http/test_http_requests.hml` (new)
- `tests/stdlib_websocket/test_websocket_basic.hml` (new)
- `tests/stdlib_websocket/test_websocket_echo.hml` (new)
- `tests/stdlib_websocket/test_websocket_server.hml` (new)

**New documentation:**
- `tests/stdlib_http/README.md` (new)
- `tests/stdlib_websocket/README.md` (new)
- `TESTING.md` (new)
- `TEST_SUITE_SUMMARY.md` (this file)

**Cleanup:**
- Removed `test_http_simple.hml`
- Removed `test_http_debug.hml`
- Removed `test_websocket_ffi.hml`

## Conclusion

The HTTP and WebSocket modules now have comprehensive test coverage with:
- ✅ **5 test files** covering 22 features
- ✅ **Detailed documentation** for each test suite
- ✅ **Clear installation instructions**
- ✅ **Troubleshooting guides**
- ✅ **Integration with main test runner**
- ✅ **Graceful degradation** (basic tests work without libwebsockets)

All tests follow best practices:
- Consistent structure
- Clear output
- Proper error handling
- Comprehensive assertions
- Good documentation
