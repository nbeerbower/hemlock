# Hemlock Networking Modules Design

> Design document for `@stdlib/net`, `@stdlib/http`, and `@stdlib/websocket`

## Philosophy

Hemlock's networking modules follow the core language philosophy:
- **Explicit over implicit** - No hidden connection pooling, retries, or buffering
- **Manual resource management** - Sockets/connections must be manually closed
- **Unsafe is a feature** - Raw socket access available when needed
- **Async-first design** - Built to work seamlessly with spawn/await
- **C-like primitives** - Low-level building blocks, high-level convenience

---

## Architecture Overview

### Three-Layer Design

```
┌─────────────────────────────────────┐
│   @stdlib/websocket (Pure Hemlock)  │  ← WebSocket protocol
├─────────────────────────────────────┤
│   @stdlib/http (Pure Hemlock)       │  ← HTTP client/server
├─────────────────────────────────────┤
│   @stdlib/net (Hemlock + C)         │  ← TCP/UDP/DNS wrappers
├─────────────────────────────────────┤
│   C Builtins (src/interpreter/net.c)│  ← POSIX socket syscalls
└─────────────────────────────────────┘
```

**Layer 1: C Builtins** - Raw socket primitives
- POSIX socket API wrappers (socket, bind, listen, accept, connect, send, recv)
- Socket object type (like File object)
- DNS resolution (getaddrinfo)
- ~500-800 lines of C

**Layer 2: @stdlib/net** - Ergonomic socket API
- TcpListener, TcpStream, UdpSocket classes
- Nice wrappers over C builtins
- Connection helpers, timeout support
- ~400-600 lines of Hemlock

**Layer 3: @stdlib/http** - HTTP protocol
- Request/Response parsing
- HTTP client (GET, POST, etc.)
- HTTP server (routing, middleware)
- ~800-1200 lines of Hemlock

**Layer 4: @stdlib/websocket** - WebSocket protocol
- WebSocket handshake (HTTP Upgrade)
- Frame encoding/decoding
- Client and server support
- ~600-900 lines of Hemlock

---

## Implementation Approach: Hybrid (C Builtins + Hemlock Wrappers)

### Why Not Pure FFI?

**Option A: Pure FFI to libc sockets**
```hemlock
// Too much boilerplate for users
let sock = ffi_socket(AF_INET, SOCK_STREAM, 0);
let addr = ffi_sockaddr_in();
addr.sin_family = AF_INET;
addr.sin_port = htons(8080);
ffi_bind(sock, addr, sizeof_sockaddr_in());
// ... too low-level
```

**Option B: FFI to external library (libcurl, libwebsockets)**
- ❌ External dependencies
- ❌ Less control over behavior
- ❌ Opaque APIs don't match Hemlock philosophy

**Option C: Hybrid (CHOSEN)**
- ✅ C builtins for performance-critical socket ops
- ✅ Hemlock wrappers for ergonomics
- ✅ No external dependencies
- ✅ Full control over behavior
- ✅ Matches existing pattern (File I/O)

---

## Phase 1: C Builtins (`src/interpreter/net.c`)

### Socket Object Type

Similar to File object, Socket is an opaque handle:

```c
typedef struct {
    int fd;              // File descriptor
    char* address;       // Bound/connected address (nullable)
    int port;            // Port number
    int domain;          // AF_INET, AF_INET6
    int type;            // SOCK_STREAM, SOCK_DGRAM
    bool closed;         // Whether socket is closed
    bool listening;      // Whether listening (server socket)
} Socket;
```

### C Builtin Functions

**Socket Creation:**
```c
Value builtin_socket_create(Value* args, int arg_count);
// socket_create(domain: i32, type: i32, protocol: i32) -> Socket
// Example: socket_create(AF_INET, SOCK_STREAM, 0)
```

**Server Operations:**
```c
Value builtin_socket_bind(Value* args, int arg_count);
// socket.bind(address: string, port: i32) -> null
// Throws on error

Value builtin_socket_listen(Value* args, int arg_count);
// socket.listen(backlog: i32) -> null

Value builtin_socket_accept(Value* args, int arg_count);
// socket.accept() -> Socket
// Returns new Socket object for client connection
```

**Client Operations:**
```c
Value builtin_socket_connect(Value* args, int arg_count);
// socket.connect(address: string, port: i32) -> null
// Throws on error (connection refused, timeout, etc.)
```

**I/O Operations:**
```c
Value builtin_socket_send(Value* args, int arg_count);
// socket.send(data: string | buffer) -> i32 (bytes sent)
// Throws on error

Value builtin_socket_recv(Value* args, int arg_count);
// socket.recv(size: i32) -> buffer
// Returns buffer with received data (may be less than size)
// Returns empty buffer on EOF
```

**UDP Operations:**
```c
Value builtin_socket_sendto(Value* args, int arg_count);
// socket.sendto(address: string, port: i32, data: string | buffer) -> i32

Value builtin_socket_recvfrom(Value* args, int arg_count);
// socket.recvfrom(size: i32) -> { data: buffer, address: string, port: i32 }
```

**DNS Resolution:**
```c
Value builtin_dns_resolve(Value* args, int arg_count);
// dns_resolve(hostname: string) -> string (IP address)
// Uses getaddrinfo(), returns first IPv4 result
// Throws on resolution failure
```

**Socket Properties (read-only):**
```c
// socket.address -> string (or null)
// socket.port -> i32
// socket.closed -> bool
// socket.fd -> i32 (file descriptor, for advanced use)
```

**Resource Management:**
```c
Value builtin_socket_close(Value* args, int arg_count);
// socket.close() -> null
// Idempotent (safe to call multiple times)
// Automatically called on Socket object destruction (if we add GC later)
```

**Socket Options:**
```c
Value builtin_socket_setsockopt(Value* args, int arg_count);
// socket.setsockopt(level: i32, option: i32, value: i32) -> null
// For SO_REUSEADDR, SO_KEEPALIVE, etc.

Value builtin_socket_set_timeout(Value* args, int arg_count);
// socket.set_timeout(seconds: f64) -> null
// Sets recv/send timeout (SO_RCVTIMEO, SO_SNDTIMEO)
```

### Constants

Export socket constants as global i32 values:

```c
// Domain constants
AF_INET = 2
AF_INET6 = 10

// Type constants
SOCK_STREAM = 1
SOCK_DGRAM = 2

// Socket options
SO_REUSEADDR = 2
SO_KEEPALIVE = 9
```

### Error Handling

All socket operations throw exceptions on errors:

```hemlock
try {
    let sock = socket_create(AF_INET, SOCK_STREAM, 0);
    sock.bind("0.0.0.0", 8080);
} catch (e) {
    print("Socket error: " + e);
    // "Failed to bind socket to 0.0.0.0:8080: Address already in use"
}
```

**Error message format:**
- Include operation name
- Include socket details (address, port)
- Include system error message (strerror)

---

## Phase 2: @stdlib/net (Hemlock Wrappers)

### Module Structure

```hemlock
// stdlib/net.hml

// TCP server socket
define TcpListener {
    _socket: object,
    address: string,
    port: i32,
}

// TCP connection stream
define TcpStream {
    _socket: object,
    peer_addr: string,
    peer_port: i32,
}

// UDP socket
define UdpSocket {
    _socket: object,
    address: string,
    port: i32,
}
```

### TcpListener API

**Constructor:**
```hemlock
fn TcpListener.bind(address: string, port: i32): TcpListener {
    let sock = socket_create(AF_INET, SOCK_STREAM, 0);
    sock.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1);  // Reuse address
    sock.bind(address, port);
    sock.listen(128);  // Backlog of 128

    return {
        _socket: sock,
        address: address,
        port: port,

        accept: fn(): TcpStream {
            let client_sock = self._socket.accept();
            return TcpStream._from_socket(client_sock);
        },

        close: fn(): null {
            self._socket.close();
            return null;
        },
    };
}
```

**Usage:**
```hemlock
import { TcpListener } from "@stdlib/net";

let listener = TcpListener.bind("0.0.0.0", 8080);
defer listener.close();

while (true) {
    let stream = listener.accept();
    // Handle connection...
    stream.close();
}
```

### TcpStream API

**Constructor (internal):**
```hemlock
fn TcpStream._from_socket(sock: object): TcpStream {
    return {
        _socket: sock,
        peer_addr: sock.address,
        peer_port: sock.port,

        read: fn(size: i32): buffer { /* ... */ },
        read_line: fn(): string { /* ... */ },
        read_all: fn(): buffer { /* ... */ },
        write: fn(data): i32 { /* ... */ },
        write_line: fn(line: string): i32 { /* ... */ },
        close: fn(): null { /* ... */ },
    };
}

fn TcpStream.connect(address: string, port: i32): TcpStream {
    let sock = socket_create(AF_INET, SOCK_STREAM, 0);
    sock.connect(address, port);
    return TcpStream._from_socket(sock);
}
```

**Usage:**
```hemlock
import { TcpStream } from "@stdlib/net";

// Client
let stream = TcpStream.connect("example.com", 80);
defer stream.close();

stream.write("GET / HTTP/1.1\r\nHost: example.com\r\n\r\n");
let response = stream.read_all();
print(response);
```

### UdpSocket API

```hemlock
fn UdpSocket.bind(address: string, port: i32): UdpSocket {
    let sock = socket_create(AF_INET, SOCK_DGRAM, 0);
    sock.bind(address, port);

    return {
        _socket: sock,
        address: address,
        port: port,

        send_to: fn(dest_addr: string, dest_port: i32, data): i32 {
            return self._socket.sendto(dest_addr, dest_port, data);
        },

        recv_from: fn(size: i32): object {
            return self._socket.recvfrom(size);
            // Returns { data: buffer, address: string, port: i32 }
        },

        close: fn(): null {
            self._socket.close();
            return null;
        },
    };
}
```

**Usage:**
```hemlock
import { UdpSocket } from "@stdlib/net";

// UDP echo server
let sock = UdpSocket.bind("0.0.0.0", 5000);
defer sock.close();

while (true) {
    let packet = sock.recv_from(1024);
    print("Received from " + packet.address + ":" + typeof(packet.port));
    sock.send_to(packet.address, packet.port, packet.data);
}
```

### Helper Functions

```hemlock
// Resolve hostname to IP
fn resolve(hostname: string): string {
    return dns_resolve(hostname);
}

// Check if address is valid IPv4
fn is_ipv4(address: string): bool {
    // Parse and validate
}

// Split "host:port" string
fn parse_address(addr: string): object {
    // Returns { host: string, port: i32 }
}
```

---

## Phase 3: @stdlib/http (HTTP Protocol)

### Module Structure

```hemlock
// stdlib/http.hml

define Request {
    method: string,      // "GET", "POST", etc.
    path: string,        // "/api/users"
    version: string,     // "HTTP/1.1"
    headers: object,     // { "Content-Type": "text/html" }
    body: string,        // Request body
}

define Response {
    version: string,     // "HTTP/1.1"
    status: i32,         // 200, 404, etc.
    reason: string,      // "OK", "Not Found"
    headers: object,     // { "Content-Type": "text/html" }
    body: string,        // Response body
}

define Client {
    timeout: f64,        // Request timeout in seconds
}

define Server {
    listener: object,    // TcpListener
    routes: array,       // Route handlers
}
```

### HTTP Client API

**Simple Interface:**
```hemlock
import { get, post } from "@stdlib/http";

// Simple GET
let resp = get("http://example.com/api/users");
print(resp.status);  // 200
print(resp.body);    // JSON response

// POST with body
let resp2 = post("http://example.com/api/users", {
    headers: { "Content-Type": "application/json" },
    body: '{"name":"Alice"}'
});
```

**Client Object:**
```hemlock
import { Client } from "@stdlib/http";

let client = Client();
client.timeout = 30.0;  // 30 second timeout

let req = {
    method: "GET",
    url: "http://example.com/api/users",
    headers: { "User-Agent": "Hemlock/0.1" },
};

let resp = client.request(req);
print(resp.status);
print(resp.headers);
print(resp.body);
```

**Implementation (simplified):**
```hemlock
fn Client.request(req: object): Response {
    // 1. Parse URL (host, port, path)
    let url_parts = parse_url(req.url);

    // 2. Resolve hostname
    let ip = resolve(url_parts.host);

    // 3. Connect TCP socket
    let stream = TcpStream.connect(ip, url_parts.port);
    defer stream.close();

    // 4. Set timeout
    if (self.timeout > 0.0) {
        stream._socket.set_timeout(self.timeout);
    }

    // 5. Build HTTP request
    let http_req = build_request(req.method, url_parts.path, req.headers, req.body);

    // 6. Send request
    stream.write(http_req);

    // 7. Read response
    let raw_response = stream.read_all();

    // 8. Parse response
    return parse_response(raw_response);
}
```

### HTTP Server API

**Simple Interface:**
```hemlock
import { Server } from "@stdlib/http";

let server = Server("0.0.0.0", 8080);

// Add routes
server.route("GET", "/", fn(req) {
    return { status: 200, body: "Hello, World!" };
});

server.route("POST", "/api/users", fn(req) {
    let user = req.body.deserialize();
    // ... save user ...
    return { status: 201, body: '{"id":123}' };
});

// Start server (blocks)
server.run();
```

**Async Server (spawn per connection):**
```hemlock
import { Server } from "@stdlib/http";

async fn handle_connection(stream: object) {
    defer stream.close();

    let raw_req = stream.read_all();
    let req = parse_request(raw_req);

    let resp = route_request(req);

    let http_resp = build_response(resp.status, resp.headers, resp.body);
    stream.write(http_resp);
}

let listener = TcpListener.bind("0.0.0.0", 8080);
while (true) {
    let stream = listener.accept();
    spawn(handle_connection, stream);  // Handle in background
}
```

**Server Methods:**
```hemlock
define Server {
    listener: object,
    routes: array,

    route: fn(method: string, path: string, handler) { /* ... */ },
    run: fn() { /* Accept loop */ },
    run_async: fn() { /* Spawn tasks for each connection */ },
}
```

### HTTP Parsing

**Request Parsing:**
```hemlock
fn parse_request(raw: string): Request {
    let lines = raw.split("\r\n");

    // Parse request line: "GET /path HTTP/1.1"
    let request_line = lines[0].split(" ");
    let method = request_line[0];
    let path = request_line[1];
    let version = request_line[2];

    // Parse headers
    let headers = {};
    let i = 1;
    while (i < lines.length && lines[i] != "") {
        let parts = lines[i].split(": ");
        headers[parts[0]] = parts[1];
        i = i + 1;
    }

    // Parse body (after empty line)
    let body = lines.slice(i + 1, lines.length).join("\r\n");

    return { method: method, path: path, version: version, headers: headers, body: body };
}
```

**Response Parsing:**
```hemlock
fn parse_response(raw: string): Response {
    // Similar to parse_request
    // Parse status line: "HTTP/1.1 200 OK"
    // Parse headers
    // Parse body
}
```

### Routing

**Route Matching:**
```hemlock
fn Server.route(method: string, path: string, handler) {
    self.routes.push({
        method: method,
        path: path,
        handler: handler
    });
}

fn Server._match_route(method: string, path: string) {
    let i = 0;
    while (i < self.routes.length) {
        let route = self.routes[i];
        if (route.method == method && route.path == path) {
            return route.handler;
        }
        i = i + 1;
    }
    return null;  // 404
}
```

---

## Phase 4: @stdlib/websocket (WebSocket Protocol)

### Module Structure

```hemlock
// stdlib/websocket.hml

define WebSocket {
    _stream: object,     // TcpStream
    _state: string,      // "connecting", "open", "closing", "closed"
}

define WebSocketServer {
    _http_server: object,
    _clients: array,
}
```

### WebSocket Client API

```hemlock
import { connect } from "@stdlib/websocket";

let ws = connect("ws://localhost:8080/chat");
defer ws.close();

ws.send("Hello, server!");

while (true) {
    let msg = ws.recv();
    if (msg == null) { break; }  // Connection closed
    print("Received: " + msg);
}
```

**Implementation:**
```hemlock
fn connect(url: string): WebSocket {
    // 1. Parse ws:// URL
    let parts = parse_ws_url(url);

    // 2. Connect TCP
    let stream = TcpStream.connect(parts.host, parts.port);

    // 3. Send HTTP Upgrade request
    let handshake = build_ws_handshake(parts.host, parts.path);
    stream.write(handshake);

    // 4. Read HTTP response
    let response = stream.read_until("\r\n\r\n");

    // 5. Verify handshake
    if (!verify_ws_handshake(response)) {
        throw "WebSocket handshake failed";
    }

    return {
        _stream: stream,
        _state: "open",

        send: fn(data: string): null { /* Encode frame, send */ },
        recv: fn(): string { /* Read frame, decode */ },
        close: fn(): null { /* Send close frame, close stream */ },
    };
}
```

### WebSocket Server API

```hemlock
import { Server } from "@stdlib/websocket";

let server = Server("0.0.0.0", 8080);

server.on_connect(fn(client) {
    print("Client connected");
});

server.on_message(fn(client, message) {
    print("Received: " + message);
    client.send("Echo: " + message);
});

server.on_disconnect(fn(client) {
    print("Client disconnected");
});

server.run();
```

### WebSocket Frame Encoding

**Frame Structure (RFC 6455):**
```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               |Masking-key, if MASK set to 1  |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+
```

**Encode Text Frame:**
```hemlock
fn encode_frame(data: string, opcode: u8): buffer {
    let payload = data.bytes();
    let len = payload.length;

    let frame = buffer(2 + len);  // Simplified (no extended length)

    frame[0] = 0x80 | opcode;  // FIN=1, opcode
    frame[1] = len;            // Payload length (< 126)

    // Copy payload
    let i = 0;
    while (i < len) {
        frame[2 + i] = payload[i];
        i = i + 1;
    }

    return frame;
}
```

---

## Phase 5: TLS/SSL Support (Future)

### Approach: FFI to OpenSSL or mbedTLS

**Option A: OpenSSL via FFI**
```hemlock
import { TlsContext, TlsStream } from "@stdlib/tls";

let ctx = TlsContext();
ctx.load_verify_locations("/etc/ssl/certs");

let stream = TcpStream.connect("example.com", 443);
let tls_stream = TlsStream(stream, ctx);
defer tls_stream.close();

tls_stream.write("GET / HTTP/1.1\r\n\r\n");
let response = tls_stream.read_all();
```

**Option B: Built-in TLS (C builtins)**
- Add TLS socket type
- Wrap OpenSSL/mbedTLS in C
- Expose as `socket_enable_tls(sock)`

**Recommendation: Start without TLS, add in Phase 5**
- Focus on HTTP first (port 80)
- Add HTTPS later (port 443)
- WebSocket + TLS = WSS support

---

## Testing Strategy

### Unit Tests

```
tests/stdlib_net/
├── tcp_client.hml          # Connect, send, recv, close
├── tcp_server.hml          # Bind, listen, accept
├── udp_socket.hml          # Bind, sendto, recvfrom
├── dns_resolve.hml         # Resolve localhost, google.com
├── socket_errors.hml       # Connection refused, timeout
└── concurrent_clients.hml  # Spawn multiple clients

tests/stdlib_http/
├── client_get.hml          # GET request
├── client_post.hml         # POST with body
├── server_basic.hml        # Simple HTTP server
├── server_routes.hml       # Multiple routes
├── parse_request.hml       # Request parsing
└── parse_response.hml      # Response parsing

tests/stdlib_websocket/
├── client.hml              # Connect, send, recv
├── server.hml              # Server with handlers
├── echo.hml                # Echo server test
└── frame_encoding.hml      # Frame encode/decode
```

### Integration Tests

**HTTP Echo Server:**
```hemlock
// tests/integration/http_echo.hml
import { Server } from "@stdlib/http";

let server = Server("127.0.0.1", 9999);
server.route("POST", "/echo", fn(req) {
    return { status: 200, body: req.body };
});

// Test: Send POST, verify response matches body
```

**WebSocket Chat:**
```hemlock
// tests/integration/ws_chat.hml
import { Server } from "@stdlib/websocket";

let clients = [];

let server = Server("127.0.0.1", 9998);
server.on_connect(fn(client) {
    clients.push(client);
});
server.on_message(fn(client, msg) {
    // Broadcast to all clients
    let i = 0;
    while (i < clients.length) {
        clients[i].send(msg);
        i = i + 1;
    }
});

// Test: Connect 2 clients, send message, verify both receive
```

---

## Documentation

### API Reference Docs

```
stdlib/docs/
├── net.md          # TcpListener, TcpStream, UdpSocket
├── http.md         # Client, Server, Request, Response
└── websocket.md    # WebSocket, Server, frame protocol
```

### Examples

```
examples/networking/
├── tcp_echo_server.hml      # Simple TCP echo
├── http_client.hml          # Fetch URL
├── http_server.hml          # HTTP file server
├── ws_chat_server.hml       # WebSocket chat
└── concurrent_downloader.hml # Async HTTP downloads
```

---

## Implementation Roadmap

### Milestone 1: Core Socket Builtins (Week 1-2)
- [ ] Create `src/interpreter/net.c`
- [ ] Implement Socket object type
- [ ] Add socket creation, bind, listen, accept, connect
- [ ] Add send, recv, sendto, recvfrom
- [ ] Add DNS resolution (dns_resolve)
- [ ] Add socket options (setsockopt, set_timeout)
- [ ] Register builtins in `builtins.c`
- [ ] Add constants (AF_INET, SOCK_STREAM, etc.)
- [ ] Write unit tests for C builtins

**Estimated effort:** ~800 lines of C, 15-20 hours

### Milestone 2: @stdlib/net Module (Week 2-3)
- [ ] Create `stdlib/net.hml`
- [ ] Implement TcpListener class
- [ ] Implement TcpStream class
- [ ] Implement UdpSocket class
- [ ] Add helper functions (resolve, parse_address)
- [ ] Write tests in `tests/stdlib_net/`
- [ ] Write documentation in `stdlib/docs/net.md`

**Estimated effort:** ~500 lines of Hemlock, 10-15 hours

### Milestone 3: @stdlib/http Module (Week 3-5)
- [ ] Create `stdlib/http.hml`
- [ ] Implement Request/Response types
- [ ] Implement HTTP request parser
- [ ] Implement HTTP response parser
- [ ] Implement Client (get, post, request)
- [ ] Implement Server (route, run, run_async)
- [ ] Add URL parsing helpers
- [ ] Write tests in `tests/stdlib_http/`
- [ ] Write documentation in `stdlib/docs/http.md`

**Estimated effort:** ~1000 lines of Hemlock, 20-25 hours

### Milestone 4: @stdlib/websocket Module (Week 5-6)
- [ ] Create `stdlib/websocket.hml`
- [ ] Implement WebSocket handshake (client/server)
- [ ] Implement frame encoding/decoding
- [ ] Implement WebSocket client (connect, send, recv, close)
- [ ] Implement WebSocket server (on_connect, on_message, on_disconnect)
- [ ] Write tests in `tests/stdlib_websocket/`
- [ ] Write documentation in `stdlib/docs/websocket.md`

**Estimated effort:** ~700 lines of Hemlock, 15-20 hours

### Milestone 5: Examples & Polish (Week 6-7)
- [ ] Write example programs (tcp_echo, http_server, ws_chat)
- [ ] Integration testing
- [ ] Performance benchmarking
- [ ] Documentation polish
- [ ] Error message improvements

**Estimated effort:** 10-15 hours

### Total Estimate: 6-7 weeks, 70-95 hours

---

## Design Decisions & Rationale

### Why Socket Objects Instead of Raw FDs?

**Alternative: Expose raw file descriptors**
```hemlock
let fd = socket(AF_INET, SOCK_STREAM, 0);
bind(fd, "0.0.0.0", 8080);
listen(fd, 128);
let client_fd = accept(fd);
```

**Chosen: Socket objects**
```hemlock
let sock = socket_create(AF_INET, SOCK_STREAM, 0);
sock.bind("0.0.0.0", 8080);
sock.listen(128);
let client_sock = sock.accept();
```

**Rationale:**
- ✅ More ergonomic (method syntax)
- ✅ Can store metadata (address, port, state)
- ✅ Matches File object pattern
- ✅ Enables better error messages
- ✅ Can add properties (sock.closed, sock.fd)

### Why Not Automatic Connection Pooling?

**Alternative: Hidden connection reuse**
```hemlock
// Silently reuses connections
get("http://example.com/1");
get("http://example.com/2");  // Reuses connection automatically
```

**Chosen: Explicit control**
```hemlock
let client = Client();
client.request({ url: "http://example.com/1" });
client.request({ url: "http://example.com/2" });  // Explicit reuse
```

**Rationale:**
- ✅ Explicit over implicit (Hemlock philosophy)
- ✅ User controls when connections are reused
- ✅ No hidden state or magic
- ✅ Simpler implementation
- ❌ Slightly more verbose

### Why Manual close() Instead of RAII?

**Alternative: Automatic cleanup**
```hemlock
{
    let sock = TcpStream.connect("example.com", 80);
    sock.write("data");
}  // Automatically closed here
```

**Chosen: Manual close with defer**
```hemlock
let sock = TcpStream.connect("example.com", 80);
defer sock.close();
sock.write("data");
// Guaranteed cleanup when function returns
```

**Rationale:**
- ✅ Explicit resource management (Hemlock philosophy)
- ✅ `defer` makes cleanup visible
- ✅ No hidden behavior
- ✅ Consistent with File I/O
- ❌ User must remember to close (but `defer` helps)

### Why Exceptions Instead of Error Codes?

**Alternative: Return error codes**
```hemlock
let result = sock.connect("example.com", 80);
if (result.error != null) {
    print("Connection failed: " + result.error);
    return;
}
```

**Chosen: Throw exceptions**
```hemlock
try {
    sock.connect("example.com", 80);
} catch (e) {
    print("Connection failed: " + e);
}
```

**Rationale:**
- ✅ Matches existing Hemlock error handling
- ✅ Cleaner code for success path
- ✅ Exceptions propagate automatically
- ✅ Consistent with File I/O
- ❌ Must use try/catch for error handling

---

## Security Considerations

### Input Validation

**Always validate untrusted input:**
```hemlock
// BAD: Trusting user input
fn connect_to_user_host(host: string) {
    let sock = TcpStream.connect(host, 80);  // Could be malicious
}

// GOOD: Validate first
fn connect_to_user_host(host: string) {
    if (!is_valid_hostname(host)) {
        throw "Invalid hostname";
    }
    let sock = TcpStream.connect(host, 80);
}
```

### Buffer Overflow Protection

**Always limit read sizes:**
```hemlock
// BAD: Unbounded read
let data = sock.read_all();  // Could exhaust memory

// GOOD: Limit size
let data = sock.read(MAX_REQUEST_SIZE);
if (data.length >= MAX_REQUEST_SIZE) {
    throw "Request too large";
}
```

### Timeout Protection

**Always set timeouts for network operations:**
```hemlock
let sock = socket_create(AF_INET, SOCK_STREAM, 0);
sock.set_timeout(30.0);  // 30 second timeout
sock.connect("example.com", 80);
```

### DoS Protection

**Limit concurrent connections:**
```hemlock
let active_connections = 0;
let MAX_CONNECTIONS = 1000;

while (true) {
    if (active_connections >= MAX_CONNECTIONS) {
        sleep(0.1);  // Back off
        continue;
    }

    let client = listener.accept();
    active_connections = active_connections + 1;
    spawn(handle_client, client);
}
```

---

## Performance Considerations

### Buffering

**Use buffering for small writes:**
```hemlock
// BAD: Many small writes
sock.write("H");
sock.write("e");
sock.write("l");
sock.write("l");
sock.write("o");

// GOOD: Batch writes
let msg = "Hello";
sock.write(msg);
```

### Reusing Buffers

**Reuse buffers for repeated reads:**
```hemlock
let buf = buffer(4096);
while (true) {
    let bytes_read = sock.recv_into(buf);  // Reuse buffer
    if (bytes_read == 0) { break; }
    process(buf.slice(0, bytes_read));
}
```

### Connection Pooling (User-Implemented)

**Users can implement connection pooling:**
```hemlock
let connection_pool = [];

fn get_connection(host: string): TcpStream {
    // Check pool first
    let i = 0;
    while (i < connection_pool.length) {
        if (connection_pool[i].host == host) {
            return connection_pool[i].stream;
        }
        i = i + 1;
    }

    // Create new connection
    let stream = TcpStream.connect(host, 80);
    connection_pool.push({ host: host, stream: stream });
    return stream;
}
```

---

## Future Enhancements

### HTTP/2 Support

- Requires binary framing layer
- Multiplexing over single connection
- Server push support
- Significant complexity increase

### Async I/O (epoll/kqueue)

- Non-blocking sockets
- Event loop integration
- Better scalability for many connections
- Requires runtime changes

### QUIC Protocol

- UDP-based transport
- Built-in encryption
- Faster connection establishment
- Future of HTTP/3

### DNS Caching

- Cache DNS results
- Reduce latency
- Configurable TTL

### HTTP Client Features

- Cookie jar
- Redirect following
- Authentication (Basic, Bearer)
- Compression (gzip, deflate)
- Chunked transfer encoding

### HTTP Server Features

- Middleware support
- Static file serving
- Request logging
- Rate limiting
- CORS support

---

## Appendix: Example Programs

### TCP Echo Server

```hemlock
import { TcpListener } from "@stdlib/net";

async fn handle_client(stream: object) {
    defer stream.close();

    while (true) {
        let data = stream.read(1024);
        if (data.length == 0) { break; }  // EOF
        stream.write(data);  // Echo back
    }
}

let listener = TcpListener.bind("0.0.0.0", 8080);
print("Listening on port 8080");

while (true) {
    let client = listener.accept();
    spawn(handle_client, client);
}
```

### HTTP File Server

```hemlock
import { Server } from "@stdlib/http";
import { read_file, exists } from "@stdlib/fs";

let server = Server("0.0.0.0", 8080);

server.route("GET", "/", fn(req) {
    return { status: 200, body: "<h1>File Server</h1>" };
});

server.route("GET", "/*", fn(req) {
    let path = "." + req.path;

    if (!exists(path)) {
        return { status: 404, body: "Not Found" };
    }

    let content = read_file(path);
    return { status: 200, body: content };
});

print("Server running on http://localhost:8080");
server.run();
```

### WebSocket Chat Server

```hemlock
import { Server } from "@stdlib/websocket";

let clients = [];

let server = Server("0.0.0.0", 8080);

server.on_connect(fn(client) {
    clients.push(client);
    print("Client connected (total: " + typeof(clients.length) + ")");
});

server.on_message(fn(client, message) {
    print("Message: " + message);

    // Broadcast to all clients
    let i = 0;
    while (i < clients.length) {
        if (clients[i] != client) {
            clients[i].send(message);
        }
        i = i + 1;
    }
});

server.on_disconnect(fn(client) {
    // Remove from clients array
    let i = 0;
    while (i < clients.length) {
        if (clients[i] == client) {
            clients.remove(i);
            break;
        }
        i = i + 1;
    }
    print("Client disconnected (total: " + typeof(clients.length) + ")");
});

print("WebSocket chat server running on ws://localhost:8080");
server.run();
```

### Concurrent HTTP Downloader

```hemlock
import { get } from "@stdlib/http";
import { write_file } from "@stdlib/fs";

let urls = [
    "http://example.com/file1.txt",
    "http://example.com/file2.txt",
    "http://example.com/file3.txt",
];

async fn download(url: string, filename: string): null {
    print("Downloading " + url);

    try {
        let resp = get(url);
        write_file(filename, resp.body);
        print("Saved " + filename);
    } catch (e) {
        print("Error downloading " + url + ": " + e);
    }

    return null;
}

// Spawn all downloads concurrently
let tasks = [];
let i = 0;
while (i < urls.length) {
    let task = spawn(download, urls[i], "file" + typeof(i) + ".txt");
    tasks.push(task);
    i = i + 1;
}

// Wait for all downloads
i = 0;
while (i < tasks.length) {
    join(tasks[i]);
    i = i + 1;
}

print("All downloads complete");
```

---

## Summary

This design provides:

1. **Layered architecture** - C builtins → @stdlib/net → @stdlib/http → @stdlib/websocket
2. **Hemlock philosophy** - Explicit, manual, unsafe where needed
3. **Async-friendly** - Works with spawn/await
4. **Practical APIs** - Ergonomic wrappers over low-level primitives
5. **Testable** - Comprehensive test coverage
6. **Extensible** - Easy to add TLS, HTTP/2, etc. later

**Next steps:**
1. Review and approve this design
2. Start with Milestone 1 (C builtins)
3. Iterate on API based on usage experience
4. Add higher-level modules incrementally

Total implementation effort: ~6-7 weeks for full stack.
