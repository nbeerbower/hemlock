# WebSocket & HTTP Stdlib Design - libwebsockets Architecture

## Overview

This document describes the unified network stack for Hemlock using **libwebsockets** as the foundation for both HTTP and WebSocket protocols.

## Architecture Decision: Replace curl with libwebsockets

### Why libwebsockets?

**Single Dependency for All Networking:**
- ✅ HTTP/HTTPS client
- ✅ HTTP/HTTPS server
- ✅ WebSocket client (ws:// and wss://)
- ✅ WebSocket server
- ✅ SSL/TLS support built-in
- ✅ Battle-tested in production systems

**Benefits over curl:**
1. **Unified API** - Same library for HTTP and WebSocket
2. **Smaller footprint** - More lightweight than curl
3. **Better WebSocket integration** - HTTP → WebSocket upgrades seamless
4. **Lower-level control** - Fits Hemlock's "explicit over implicit" philosophy
5. **No external dependencies** - Single .so file

**Trade-offs:**
- Less ubiquitous than curl (but easy to install: `apt-get install libwebsockets-dev`)
- Fewer HTTP features (no FTP, SMTP, etc.) - but we only need HTTP anyway
- More complex API - requires C wrapper

## Implementation Strategy

### Three-Layer Architecture

```
┌─────────────────────────────────────────────┐
│   Hemlock Code (@stdlib/http, @stdlib/ws)  │  <- Pure Hemlock API
├─────────────────────────────────────────────┤
│   FFI Declarations (extern fn)              │  <- Hemlock FFI layer
├─────────────────────────────────────────────┤
│   C Wrapper (lws_wrapper.so)                │  <- Simplified C API
├─────────────────────────────────────────────┤
│   libwebsockets                             │  <- Native library
└─────────────────────────────────────────────┘
```

**Layer 1: Native Library (libwebsockets)**
- Installed system library (`libwebsockets.so`)
- Provides low-level HTTP and WebSocket implementation
- Handles SSL/TLS, connection management, protocol details

**Layer 2: C Wrapper (`stdlib/c/lws_wrapper.c` → `lws_wrapper.so`)**
- Simplifies libwebsockets API for FFI consumption
- Handles callbacks, memory management, event loops
- Exposes clean C functions that Hemlock can call
- Abstracts away complexity (context creation, protocol structs, etc.)

**Layer 3: FFI Declarations (Hemlock)**
- `extern fn` declarations in Hemlock modules
- Type-safe bindings to C wrapper functions
- Manual memory management (explicit free calls)

**Layer 4: Hemlock API (`@stdlib/http`, `@stdlib/websocket`)**
- User-friendly, idiomatic Hemlock API
- Object-oriented wrappers (WebSocket, WebSocketServer, etc.)
- Methods with `defer` cleanup support
- Exception-based error handling

## Module Structure

```
stdlib/
├── http.hml                    # HTTP client (replaces curl version)
├── websocket.hml               # WebSocket client/server
├── c/
│   └── lws_wrapper.c          # C FFI wrapper
│       └── lws_wrapper.so     # Compiled shared library
└── docs/
    ├── http.md                # HTTP documentation
    └── websocket.md           # WebSocket documentation

tests/
├── stdlib_http/               # HTTP tests
└── stdlib_websocket/          # WebSocket tests
    ├── client.hml
    ├── server.hml
    └── echo_test.hml
```

## API Design

### @stdlib/http - HTTP Client

**High-Level API (User-Facing):**
```hemlock
import { get, post, fetch, post_json } from "@stdlib/http";

// Simple GET
let response = get("https://api.github.com/users/octocat", null);
print(response.status_code);  // 200
print(response.body);          // JSON response

// POST with JSON
let data = { name: "Alice", age: 30 };
let resp = post_json("https://api.example.com/users", data);

// Quick fetch
let html = fetch("https://example.com");
```

**Response Object:**
```hemlock
{
    status_code: i32,     // HTTP status (200, 404, etc.)
    headers: string,      // Response headers (newline-separated)
    body: string,         // Response body
}
```

### @stdlib/websocket - WebSocket Client/Server

**Client API:**
```hemlock
import { WebSocket } from "@stdlib/websocket";

let ws = WebSocket("wss://echo.websocket.org");
defer ws.close();

// Send text
ws.send_text("Hello, WebSocket!");

// Receive (blocking)
let msg = ws.recv();
if (msg != null && msg.type == "text") {
    print("Received: " + msg.data);
}

// Receive with timeout
let msg2 = ws.recv(5000);  // 5 second timeout
if (msg2 == null) {
    print("Timeout");
}
```

**Server API:**
```hemlock
import { WebSocketServer } from "@stdlib/websocket";

let server = WebSocketServer("0.0.0.0", 8080);
defer server.close();

print("WebSocket server listening on :8080");

while (true) {
    let conn = server.accept();  // Blocks until client connects
    spawn(handle_client, conn);
}

async fn handle_client(conn) {
    defer conn.close();

    while (true) {
        let msg = conn.recv();
        if (msg == null) { break; }  // Connection closed

        if (msg.type == "text") {
            print("Client said: " + msg.data);
            conn.send_text("Echo: " + msg.data);
        }
    }
}
```

**Message Object:**
```hemlock
{
    type: string,      // "text", "binary", "ping", "pong", "close"
    data: string,      // Text content (or null)
    binary: buffer,    // Binary content (or null)
}
```

## C Wrapper API

### HTTP Functions

```c
// HTTP GET request
http_response_t* lws_http_get(const char *url);

// HTTP POST request
http_response_t* lws_http_post(const char *url, const char *body, const char *content_type);

// Generic HTTP request
http_response_t* lws_http_request(const char *method, const char *url,
                                   const char *body, const char **headers);

// Free response
void lws_http_response_free(http_response_t *resp);

// Response accessors
int lws_response_status(http_response_t *resp);
const char* lws_response_body(http_response_t *resp);
const char* lws_response_headers(http_response_t *resp);
```

### WebSocket Functions

```c
// Client
ws_connection_t* lws_ws_connect(const char *url);
int lws_ws_send_text(ws_connection_t *conn, const char *text);
int lws_ws_send_binary(ws_connection_t *conn, const unsigned char *data, size_t len);
ws_message_t* lws_ws_recv(ws_connection_t *conn, int timeout_ms);
void lws_ws_close(ws_connection_t *conn);
int lws_ws_is_closed(ws_connection_t *conn);

// Server
ws_server_t* lws_ws_server_create(const char *host, int port);
ws_connection_t* lws_ws_server_accept(ws_server_t *server, int timeout_ms);
void lws_ws_server_close(ws_server_t *server);

// Message accessors
int lws_msg_type(ws_message_t *msg);
const char* lws_msg_text(ws_message_t *msg);
const unsigned char* lws_msg_binary(ws_message_t *msg);
size_t lws_msg_len(ws_message_t *msg);
void lws_msg_free(ws_message_t *msg);
```

## Implementation Plan

### Phase 1: C Wrapper Development
- [x] Create `stdlib/c/lws_wrapper.c` with HTTP support
- [x] Add WebSocket client support
- [ ] Add WebSocket server support
- [ ] Implement proper URL parsing
- [ ] Add SSL certificate validation options
- [ ] Add custom header support for HTTP
- [ ] Add timeout configuration

### Phase 2: Hemlock FFI Bindings
- [x] Create `stdlib/http.hml` with FFI declarations
- [x] Create `stdlib/websocket.hml` with FFI declarations
- [ ] Implement proper error handling and exceptions
- [ ] Add memory cleanup with `defer`
- [ ] Test all FFI type conversions

### Phase 3: High-Level API
- [ ] Implement HTTP convenience functions (fetch, post_json, etc.)
- [ ] Implement WebSocket message parsing
- [ ] Add connection state management
- [ ] Implement graceful close for WebSockets
- [ ] Add ping/pong handling

### Phase 4: Testing & Documentation
- [ ] Write HTTP client tests
- [ ] Write WebSocket client tests
- [ ] Write WebSocket server tests
- [ ] Test SSL/TLS connections
- [ ] Write comprehensive documentation
- [ ] Add usage examples

### Phase 5: Integration & Cleanup
- [ ] Remove old curl-based HTTP implementation
- [ ] Update CLAUDE.md with new stdlib modules
- [ ] Add installation instructions for libwebsockets
- [ ] Performance testing and optimization

## Build Integration

### Makefile Changes

```makefile
# Add libwebsockets wrapper to build
STDLIB_LWS = stdlib/c/lws_wrapper.so

$(STDLIB_LWS): stdlib/c/lws_wrapper.c
	$(CC) -shared -fPIC -o $@ $< -lwebsockets $(CFLAGS)

# Add to stdlib target
stdlib: $(STDLIB_LWS)

# Add to clean target
clean:
	rm -f $(STDLIB_LWS)
```

### Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get install libwebsockets-dev
```

**macOS:**
```bash
brew install libwebsockets
```

**Arch Linux:**
```bash
sudo pacman -S libwebsockets
```

## Security Considerations

### SSL/TLS Support

libwebsockets provides built-in SSL/TLS:
- Supports `https://` and `wss://` URLs
- Uses system certificate store by default
- Can configure custom certificates

**Example SSL configuration in C wrapper:**
```c
connect_info.ssl_connection = LCCSCF_USE_SSL |
                               LCCSCF_ALLOW_SELFSIGNED |
                               LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
```

### Certificate Validation

Default behavior: **Strict validation**
- Verifies server certificates against system CA store
- Checks hostname matches certificate

Optional relaxed modes (for development):
- `LCCSCF_ALLOW_SELFSIGNED` - Accept self-signed certs
- `LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK` - Skip hostname check

**Hemlock API (planned):**
```hemlock
// Strict (default)
let ws = WebSocket("wss://secure.example.com");

// Development mode (unsafe!)
let ws_dev = WebSocket("wss://localhost:8080", { verify_ssl: false });
```

## Performance Characteristics

### HTTP Requests
- **Blocking by design** - Fits Hemlock's explicit model
- **Connection pooling** - None (new connection per request)
- **Typical latency** - 50-200ms for remote requests
- **Memory usage** - ~8KB per request + response body size

### WebSocket Connections
- **Long-lived** - Designed for persistent connections
- **Memory per connection** - ~16KB + buffer sizes
- **Throughput** - Depends on message size and frequency
- **Concurrent connections** - Limited by OS (typically 1000s)

## Comparison with Other Languages

### Python (websockets library)
```python
# Python
import websockets

async with websockets.connect("wss://echo.websocket.org") as ws:
    await ws.send("Hello")
    msg = await ws.recv()
```

### Hemlock
```hemlock
// Hemlock
import { WebSocket } from "@stdlib/websocket";

let ws = WebSocket("wss://echo.websocket.org");
defer ws.close();

ws.send_text("Hello");
let msg = ws.recv();
```

**Key differences:**
- Hemlock is **blocking by default** (explicit)
- Hemlock requires **manual cleanup** (defer)
- Hemlock uses **OS threads** for concurrency (spawn)

## Future Enhancements

### Planned Features
- [ ] HTTP/2 support (when libwebsockets adds it)
- [ ] WebSocket compression (permessage-deflate)
- [ ] Proxy support (HTTP and SOCKS)
- [ ] Request/response interceptors
- [ ] Connection pooling for HTTP
- [ ] Streaming responses (chunked transfer)

### Possible Features
- [ ] HTTP server (building on libwebsockets)
- [ ] Server-Sent Events (SSE)
- [ ] QUIC/HTTP/3 (future libwebsockets support)

## Hemlock Philosophy Alignment

✅ **Explicit over implicit**
- Blocking calls by default
- Manual connection management (defer)
- No hidden event loops

✅ **Manual memory management**
- Explicit `defer ws.close()`
- FFI wrapper manages internal buffers
- User controls object lifetimes

✅ **No magic**
- Direct mapping to libwebsockets behavior
- Errors propagate as exceptions
- No automatic reconnection

✅ **Educational**
- Shows how protocols work
- Demonstrates FFI patterns
- C wrapper is readable and teachable

## Testing Strategy

### Unit Tests
- URL parsing
- Message encoding/decoding
- Error handling
- Memory cleanup

### Integration Tests
- HTTP GET/POST against real endpoints
- WebSocket echo server
- SSL/TLS connections
- Concurrent connections

### Stress Tests
- Many simultaneous WebSocket connections
- Large message sizes
- Rapid connect/disconnect cycles

## Documentation

### User Documentation
- Getting started guide
- API reference
- Common patterns
- Error handling
- SSL/TLS configuration

### Developer Documentation
- C wrapper implementation details
- FFI binding patterns
- Memory management strategy
- Testing guide

## Migration from curl

### For Existing Code

**Old (curl via exec):**
```hemlock
import { get } from "@stdlib/http";  // Old version

let response = get("https://api.github.com", null);
// Works but shells out to curl
```

**New (libwebsockets FFI):**
```hemlock
import { get } from "@stdlib/http";  // New version

let response = get("https://api.github.com", null);
// Same API, native FFI implementation
```

**API is 100% compatible** - Just replace the implementation!

## Conclusion

Replacing curl with libwebsockets provides:
1. **Unified foundation** for HTTP and WebSocket
2. **Better integration** between protocols
3. **Smaller dependency footprint**
4. **More control** and lower-level access
5. **Production-ready** SSL/TLS support

This aligns perfectly with Hemlock's philosophy of explicit, manual control while providing powerful networking capabilities.
