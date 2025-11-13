#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

// ========== BUILTIN FUNCTIONS ==========

// Helper: Get the size of a type
static int get_type_size(TypeKind kind) {
    switch (kind) {
        case TYPE_I8:
        case TYPE_U8:
            return 1;
        case TYPE_I16:
        case TYPE_U16:
            return 2;
        case TYPE_I32:
        case TYPE_U32:
        case TYPE_F32:
            return 4;
        case TYPE_I64:
        case TYPE_U64:
        case TYPE_F64:
            return 8;
        case TYPE_PTR:
        case TYPE_BUFFER:
            return sizeof(void*);  // 8 on 64-bit systems
        case TYPE_BOOL:
            return sizeof(int);    // bool is stored as int
        case TYPE_STRING:
            return sizeof(String*); // pointer to String struct
        default:
            fprintf(stderr, "Runtime error: Cannot get size of this type\n");
            exit(1);
    }
}


static Value builtin_print(Value *args, int num_args, ExecutionContext *ctx) {
    (void)ctx;  // Unused
    if (num_args != 1) {
        fprintf(stderr, "Runtime error: print() expects 1 argument\n");
        exit(1);
    }

    print_value(args[0]);
    printf("\n");
    return val_null();
}

static Value builtin_alloc(Value *args, int num_args, ExecutionContext *ctx) {
    (void)ctx;  // Unused
    if (num_args != 1) {
        fprintf(stderr, "Runtime error: alloc() expects 1 argument (size in bytes)\n");
        exit(1);
    }

    if (!is_integer(args[0])) {
        fprintf(stderr, "Runtime error: alloc() size must be an integer\n");
        exit(1);
    }

    int32_t size = value_to_int(args[0]);

    if (size <= 0) {
        fprintf(stderr, "Runtime error: alloc() size must be positive\n");
        exit(1);
    }

    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Runtime error: alloc() failed to allocate memory\n");
        exit(1);
    }

    return val_ptr(ptr);
}

static Value builtin_free(Value *args, int num_args, ExecutionContext *ctx) {
    (void)ctx;  // Unused
    if (num_args != 1) {
        fprintf(stderr, "Runtime error: free() expects 1 argument (pointer or buffer)\n");
        exit(1);
    }

    if (args[0].type == VAL_PTR) {
        free(args[0].as.as_ptr);
        return val_null();
    } else if (args[0].type == VAL_BUFFER) {
        buffer_free(args[0].as.as_buffer);
        return val_null();
    } else {
        fprintf(stderr, "Runtime error: free() requires a pointer or buffer\n");
        exit(1);
    }
}

static Value builtin_memset(Value *args, int num_args, ExecutionContext *ctx) {
    (void)ctx;  // Unused
    if (num_args != 3) {
        fprintf(stderr, "Runtime error: memset() expects 3 arguments (ptr, byte, size)\n");
        exit(1);
    }

    if (args[0].type != VAL_PTR) {
        fprintf(stderr, "Runtime error: memset() requires pointer as first argument\n");
        exit(1);
    }

    if (!is_integer(args[1]) || !is_integer(args[2])) {
        fprintf(stderr, "Runtime error: memset() byte and size must be integers\n");
        exit(1);
    }

    void *ptr = args[0].as.as_ptr;
    int byte = value_to_int(args[1]);
    int size = value_to_int(args[2]);

    memset(ptr, byte, size);
    return val_null();
}

static Value builtin_memcpy(Value *args, int num_args, ExecutionContext *ctx) {
    (void)ctx;  // Unused
    if (num_args != 3) {
        fprintf(stderr, "Runtime error: memcpy() expects 3 arguments (dest, src, size)\n");
        exit(1);
    }

    if (args[0].type != VAL_PTR || args[1].type != VAL_PTR) {
        fprintf(stderr, "Runtime error: memcpy() requires pointers for dest and src\n");
        exit(1);
    }

    if (!is_integer(args[2])) {
        fprintf(stderr, "Runtime error: memcpy() size must be an integer\n");
        exit(1);
    }

    void *dest = args[0].as.as_ptr;
    void *src = args[1].as.as_ptr;
    int size = value_to_int(args[2]);

    memcpy(dest, src, size);
    return val_null();
}

static Value builtin_sizeof(Value *args, int num_args, ExecutionContext *ctx) {
    (void)ctx;  // Unused
    if (num_args != 1) {
        fprintf(stderr, "Runtime error: sizeof() expects 1 argument (type)\n");
        exit(1);
    }

    if (args[0].type != VAL_TYPE) {
        fprintf(stderr, "Runtime error: sizeof() requires a type argument\n");
        exit(1);
    }

    TypeKind kind = args[0].as.as_type;
    int size = get_type_size(kind);
    return val_i32(size);
}

static Value builtin_buffer(Value *args, int num_args, ExecutionContext *ctx) {
    (void)ctx;  // Unused
    if (num_args != 1) {
        fprintf(stderr, "Runtime error: buffer() expects 1 argument (size in bytes)\n");
        exit(1);
    }

    if (!is_integer(args[0])) {
        fprintf(stderr, "Runtime error: buffer() size must be an integer\n");
        exit(1);
    }

    int32_t size = value_to_int(args[0]);
    return val_buffer(size);
}

static Value builtin_talloc(Value *args, int num_args, ExecutionContext *ctx) {
    (void)ctx;  // Unused
    if (num_args != 2) {
        fprintf(stderr, "Runtime error: talloc() expects 2 arguments (type, count)\n");
        exit(1);
    }

    if (args[0].type != VAL_TYPE) {
        fprintf(stderr, "Runtime error: talloc() first argument must be a type\n");
        exit(1);
    }

    if (!is_integer(args[1])) {
        fprintf(stderr, "Runtime error: talloc() count must be an integer\n");
        exit(1);
    }

    TypeKind type = args[0].as.as_type;
    int32_t count = value_to_int(args[1]);

    if (count <= 0) {
        fprintf(stderr, "Runtime error: talloc() count must be positive\n");
        exit(1);
    }

    int elem_size = get_type_size(type);
    size_t total_size = (size_t)elem_size * (size_t)count;

    void *ptr = malloc(total_size);
    if (ptr == NULL) {
        fprintf(stderr, "Runtime error: talloc() failed to allocate memory\n");
        exit(1);
    }

    return val_ptr(ptr);
}

static Value builtin_realloc(Value *args, int num_args, ExecutionContext *ctx) {
    (void)ctx;  // Unused
    if (num_args != 2) {
        fprintf(stderr, "Runtime error: realloc() expects 2 arguments (ptr, new_size)\n");
        exit(1);
    }

    if (args[0].type != VAL_PTR) {
        fprintf(stderr, "Runtime error: realloc() first argument must be a pointer\n");
        exit(1);
    }

    if (!is_integer(args[1])) {
        fprintf(stderr, "Runtime error: realloc() new_size must be an integer\n");
        exit(1);
    }

    void *old_ptr = args[0].as.as_ptr;
    int32_t new_size = value_to_int(args[1]);

    if (new_size <= 0) {
        fprintf(stderr, "Runtime error: realloc() new_size must be positive\n");
        exit(1);
    }

    void *new_ptr = realloc(old_ptr, new_size);
    if (new_ptr == NULL) {
        fprintf(stderr, "Runtime error: realloc() failed to allocate memory\n");
        exit(1);
    }

    return val_ptr(new_ptr);
}

static Value builtin_typeof(Value *args, int num_args, ExecutionContext *ctx) {
    (void)ctx;  // Unused
    if (num_args != 1) {
        fprintf(stderr, "Runtime error: typeof() expects 1 argument\n");
        exit(1);
    }

    const char *type_name;
    switch (args[0].type) {
        case VAL_I8:
            type_name = "i8";
            break;
        case VAL_I16:
            type_name = "i16";
            break;
        case VAL_I32:
            type_name = "i32";
            break;
        case VAL_I64:
            type_name = "i64";
            break;
        case VAL_U8:
            type_name = "u8";
            break;
        case VAL_U16:
            type_name = "u16";
            break;
        case VAL_U32:
            type_name = "u32";
            break;
        case VAL_U64:
            type_name = "u64";
            break;
        case VAL_F32:
            type_name = "f32";
            break;
        case VAL_F64:
            type_name = "f64";
            break;
        case VAL_BOOL:
            type_name = "bool";
            break;
        case VAL_STRING:
            type_name = "string";
            break;
        case VAL_RUNE:
            type_name = "rune";
            break;
        case VAL_PTR:
            type_name = "ptr";
            break;
        case VAL_BUFFER:
            type_name = "buffer";
            break;
        case VAL_ARRAY:
            type_name = "array";
            break;
        case VAL_FILE:
            type_name = "file";
            break;
        case VAL_NULL:
            type_name = "null";
            break;
        case VAL_FUNCTION:
            type_name = "function";
            break;
        case VAL_BUILTIN_FN:
            type_name = "builtin";
            break;
        case VAL_OBJECT:
            // Check if object has a custom type name
            if (args[0].as.as_object->type_name) {
                type_name = args[0].as.as_object->type_name;
            } else {
                type_name = "object";
            }
            break;
        case VAL_TYPE:
            type_name = "type";
            break;
        default:
            type_name = "unknown";
            break;
    }

    return val_string(type_name);
}

// ========== I/O BUILTIN FUNCTIONS ==========
// Note: File/array method handlers are in io.c


static Value builtin_assert(Value *args, int num_args, ExecutionContext *ctx) {
    if (num_args < 1 || num_args > 2) {
        fprintf(stderr, "Runtime error: assert() expects 1-2 arguments (condition, [message])\n");
        exit(1);
    }

    // Check if condition is truthy
    int is_truthy = 0;
    switch (args[0].type) {
        case VAL_I8:
            is_truthy = args[0].as.as_i8 != 0;
            break;
        case VAL_I16:
            is_truthy = args[0].as.as_i16 != 0;
            break;
        case VAL_I32:
            is_truthy = args[0].as.as_i32 != 0;
            break;
        case VAL_U8:
            is_truthy = args[0].as.as_u8 != 0;
            break;
        case VAL_U16:
            is_truthy = args[0].as.as_u16 != 0;
            break;
        case VAL_U32:
            is_truthy = args[0].as.as_u32 != 0;
            break;
        case VAL_F32:
            is_truthy = args[0].as.as_f32 != 0.0f;
            break;
        case VAL_F64:
            is_truthy = args[0].as.as_f64 != 0.0;
            break;
        case VAL_BOOL:
            is_truthy = args[0].as.as_bool;
            break;
        case VAL_NULL:
            is_truthy = 0;
            break;
        case VAL_STRING:
            // Non-empty string is truthy
            is_truthy = args[0].as.as_string->length > 0;
            break;
        case VAL_PTR:
            is_truthy = args[0].as.as_ptr != NULL;
            break;
        default:
            // All other types (objects, arrays, functions, etc.) are truthy
            is_truthy = 1;
            break;
    }

    // If condition is false, throw exception
    if (!is_truthy) {
        Value exception_msg;
        if (num_args == 2) {
            // Use provided message
            exception_msg = args[1];
        } else {
            // Use default message
            exception_msg = val_string("assertion failed");
        }

        ctx->exception_state.exception_value = exception_msg;
        ctx->exception_state.is_throwing = 1;
    }

    return val_null();
}

// ========== ASYNC/CONCURRENCY BUILTINS ==========

// Global task ID counter
static int next_task_id = 1;

// Thread wrapper function that executes a task
static void* task_thread_wrapper(void* arg) {
    Task *task = (Task*)arg;
    Function *fn = task->function;

    // Mark as running
    task->state = TASK_RUNNING;

    // Create new environment for function execution
    Environment *func_env = env_new(task->env);

    // Bind parameters
    for (int i = 0; i < fn->num_params && i < task->num_args; i++) {
        Value arg = task->args[i];
        // Type check if parameter has type annotation
        if (fn->param_types[i]) {
            arg = convert_to_type(arg, fn->param_types[i], func_env, task->ctx);
        }
        env_define(func_env, fn->param_names[i], arg, 0, task->ctx);
    }

    // Execute function body
    eval_stmt(fn->body, func_env, task->ctx);

    // Get return value
    Value result = val_null();
    if (task->ctx->return_state.is_returning) {
        result = task->ctx->return_state.return_value;
        task->ctx->return_state.is_returning = 0;
    }

    // Store result in task
    task->result = malloc(sizeof(Value));
    *task->result = result;
    task->state = TASK_COMPLETED;

    // Clean up function environment
    env_free(func_env);

    return NULL;
}

static Value builtin_spawn(Value *args, int num_args, ExecutionContext *ctx) {
    (void)ctx;  // Not used in spawn

    if (num_args < 1) {
        fprintf(stderr, "Runtime error: spawn() expects at least 1 argument (async function)\n");
        exit(1);
    }

    Value func_val = args[0];

    if (func_val.type != VAL_FUNCTION) {
        fprintf(stderr, "Runtime error: spawn() expects an async function\n");
        exit(1);
    }

    Function *fn = func_val.as.as_function;

    if (!fn->is_async) {
        fprintf(stderr, "Runtime error: spawn() requires an async function\n");
        exit(1);
    }

    // Create task with remaining args as function arguments
    Value *task_args = NULL;
    int task_num_args = num_args - 1;

    if (task_num_args > 0) {
        task_args = malloc(sizeof(Value) * task_num_args);
        for (int i = 0; i < task_num_args; i++) {
            task_args[i] = args[i + 1];
        }
    }

    // Create task
    Task *task = task_new(next_task_id++, fn, task_args, task_num_args, fn->closure_env);

    // Allocate pthread_t
    task->thread = malloc(sizeof(pthread_t));
    if (!task->thread) {
        fprintf(stderr, "Runtime error: Memory allocation failed\n");
        exit(1);
    }

    // Create thread to execute task
    int rc = pthread_create((pthread_t*)task->thread, NULL, task_thread_wrapper, task);
    if (rc != 0) {
        fprintf(stderr, "Runtime error: Failed to create thread: %d\n", rc);
        exit(1);
    }

    return val_task(task);
}

static Value builtin_join(Value *args, int num_args, ExecutionContext *ctx) {
    if (num_args != 1) {
        fprintf(stderr, "Runtime error: join() expects 1 argument (task handle)\n");
        exit(1);
    }

    Value task_val = args[0];

    if (task_val.type != VAL_TASK) {
        fprintf(stderr, "Runtime error: join() expects a task handle\n");
        exit(1);
    }

    Task *task = task_val.as.as_task;

    if (task->joined) {
        fprintf(stderr, "Runtime error: task handle already joined\n");
        exit(1);
    }

    if (task->detached) {
        fprintf(stderr, "Runtime error: cannot join detached task\n");
        exit(1);
    }

    // Mark as joined
    task->joined = 1;

    // Wait for thread to complete
    if (task->thread) {
        int rc = pthread_join(*(pthread_t*)task->thread, NULL);
        if (rc != 0) {
            fprintf(stderr, "Runtime error: pthread_join failed: %d\n", rc);
            exit(1);
        }
    }

    // Check if task threw an exception
    if (task->ctx->exception_state.is_throwing) {
        // Re-throw the exception in the current context
        ctx->exception_state = task->ctx->exception_state;
        return val_null();
    }

    // Return the result
    if (task->result) {
        return *task->result;
    }

    return val_null();
}

static Value builtin_detach(Value *args, int num_args, ExecutionContext *ctx) {
    // detach() is like spawn() but detaches the thread
    Value task = builtin_spawn(args, num_args, ctx);

    // Detach the thread (fire and forget)
    if (task.type == VAL_TASK) {
        Task *t = task.as.as_task;
        t->detached = 1;

        if (t->thread) {
            int rc = pthread_detach(*(pthread_t*)t->thread);
            if (rc != 0) {
                fprintf(stderr, "Runtime error: pthread_detach failed: %d\n", rc);
                exit(1);
            }
        }

        // Note: We don't free the task here - it will clean itself up
        // when the thread completes. In a production system, we'd have
        // a cleanup mechanism.
    }

    return val_null();
}

static Value builtin_channel(Value *args, int num_args, ExecutionContext *ctx) {
    (void)ctx;

    int capacity = 0;  // unbuffered by default

    if (num_args > 0) {
        if (args[0].type != VAL_I32 && args[0].type != VAL_U32) {
            fprintf(stderr, "Runtime error: channel() capacity must be an integer\n");
            exit(1);
        }
        capacity = value_to_int(args[0]);

        if (capacity < 0) {
            fprintf(stderr, "Runtime error: channel() capacity cannot be negative\n");
            exit(1);
        }
    }

    Channel *ch = channel_new(capacity);
    return val_channel(ch);
}

// Structure to hold builtin function info
typedef struct {
    const char *name;
    BuiltinFn fn;
} BuiltinInfo;

static BuiltinInfo builtins[] = {
    {"print", builtin_print},
    {"alloc", builtin_alloc},
    {"talloc", builtin_talloc},
    {"realloc", builtin_realloc},
    {"free", builtin_free},
    {"memset", builtin_memset},
    {"memcpy", builtin_memcpy},
    {"sizeof", builtin_sizeof},
    {"buffer", builtin_buffer},
    {"typeof", builtin_typeof},
    {"read_line", builtin_read_line},
    {"eprint", builtin_eprint},
    {"open", builtin_open},
    {"assert", builtin_assert},
    {"spawn", builtin_spawn},
    {"join", builtin_join},
    {"detach", builtin_detach},
    {"channel", builtin_channel},
    {NULL, NULL}  // Sentinel
};

Value val_builtin_fn(BuiltinFn fn) {
    Value v;
    v.type = VAL_BUILTIN_FN;
    v.as.as_builtin_fn = fn;
    return v;
}

void register_builtins(Environment *env, int argc, char **argv, ExecutionContext *ctx) {
    // Register type constants FIRST for use with sizeof() and talloc()
    // These must be registered before builtin functions to avoid conflicts
    env_set(env, "i8", val_type(TYPE_I8), ctx);
    env_set(env, "i16", val_type(TYPE_I16), ctx);
    env_set(env, "i32", val_type(TYPE_I32), ctx);
    env_set(env, "u8", val_type(TYPE_U8), ctx);
    env_set(env, "u16", val_type(TYPE_U16), ctx);
    env_set(env, "u32", val_type(TYPE_U32), ctx);
    env_set(env, "f32", val_type(TYPE_F32), ctx);
    env_set(env, "f64", val_type(TYPE_F64), ctx);
    env_set(env, "ptr", val_type(TYPE_PTR), ctx);

    // Type aliases
    env_set(env, "integer", val_type(TYPE_I32), ctx);
    env_set(env, "number", val_type(TYPE_F64), ctx);
    env_set(env, "byte", val_type(TYPE_U8), ctx);

    // Register builtin functions (may overwrite some type names if there are conflicts)
    for (int i = 0; builtins[i].name != NULL; i++) {
        env_set(env, builtins[i].name, val_builtin_fn(builtins[i].fn), ctx);
    }

    // Register command-line arguments as 'args' array
    Array *args_array = array_new();
    for (int i = 0; i < argc; i++) {
        array_push(args_array, val_string(argv[i]));
    }
    env_set(env, "args", val_array(args_array), ctx);
}
