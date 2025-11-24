// Stub websockets.c - compiles without libwebsockets
// All functions return errors indicating websockets not available

#include "internal.h"

// HTTP stubs
Value builtin_lws_http_get(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    runtime_error(ctx, "HTTP support not available (libwebsockets not installed)");
    return val_null();
}

Value builtin_lws_http_post(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    runtime_error(ctx, "HTTP support not available (libwebsockets not installed)");
    return val_null();
}

Value builtin_lws_response_status(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    runtime_error(ctx, "HTTP support not available (libwebsockets not installed)");
    return val_null();
}

Value builtin_lws_response_body(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    runtime_error(ctx, "HTTP support not available (libwebsockets not installed)");
    return val_null();
}

Value builtin_lws_response_headers(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    runtime_error(ctx, "HTTP support not available (libwebsockets not installed)");
    return val_null();
}

Value builtin_lws_response_free(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    return val_null();
}

// WebSocket stubs
Value builtin_lws_ws_connect(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    runtime_error(ctx, "WebSocket support not available (libwebsockets not installed)");
    return val_null();
}

Value builtin_lws_ws_send_text(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    runtime_error(ctx, "WebSocket support not available (libwebsockets not installed)");
    return val_null();
}

Value builtin_lws_ws_recv(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    runtime_error(ctx, "WebSocket support not available (libwebsockets not installed)");
    return val_null();
}

Value builtin_lws_msg_type(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    runtime_error(ctx, "WebSocket support not available (libwebsockets not installed)");
    return val_null();
}

Value builtin_lws_msg_text(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    runtime_error(ctx, "WebSocket support not available (libwebsockets not installed)");
    return val_null();
}

Value builtin_lws_msg_len(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    runtime_error(ctx, "WebSocket support not available (libwebsockets not installed)");
    return val_null();
}

Value builtin_lws_msg_free(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    return val_null();
}

Value builtin_lws_ws_close(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    return val_null();
}

Value builtin_lws_ws_is_closed(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    return val_bool(1);
}

// WebSocket server stubs
Value builtin_lws_ws_server_create(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    runtime_error(ctx, "WebSocket server not available (libwebsockets not installed)");
    return val_null();
}

Value builtin_lws_ws_server_accept(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    runtime_error(ctx, "WebSocket server not available (libwebsockets not installed)");
    return val_null();
}

Value builtin_lws_ws_server_close(Value *args, int num_args, ExecutionContext *ctx) {
    (void)args; (void)num_args;
    return val_null();
}
