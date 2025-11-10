#include "interpreter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ========== HELPERS ==========

// Helper: Check if a value is any integer type
static int is_integer(Value val) {
    return val.type == VAL_I8 || val.type == VAL_I16 || val.type == VAL_I32 ||
           val.type == VAL_U8 || val.type == VAL_U16 || val.type == VAL_U32;
}

// Helper: Check if a value is any float type
static int is_float(Value val) {
    return val.type == VAL_F32 || val.type == VAL_F64;
}

// Helper: Check if a value is any numeric type
static int is_numeric(Value val) {
    return is_integer(val) || is_float(val);
}

// Helper: Convert any integer to i32 (for operations)
static int32_t value_to_int(Value val) {
    switch (val.type) {
        case VAL_I8: return val.as.as_i8;
        case VAL_I16: return val.as.as_i16;
        case VAL_I32: return val.as.as_i32;
        case VAL_U8: return val.as.as_u8;
        case VAL_U16: return val.as.as_u16;
        case VAL_U32: return (int32_t)val.as.as_u32;  // potential overflow
        case VAL_BOOL: return val.as.as_bool;
        default:
            fprintf(stderr, "Runtime error: Cannot convert to int\n");
            exit(1);
    }
}

// Helper: Convert any numeric to f64 (for operations)
static double value_to_float(Value val) {
    switch (val.type) {
        case VAL_I8: return (double)val.as.as_i8;
        case VAL_I16: return (double)val.as.as_i16;
        case VAL_I32: return (double)val.as.as_i32;
        case VAL_U8: return (double)val.as.as_u8;
        case VAL_U16: return (double)val.as.as_u16;
        case VAL_U32: return (double)val.as.as_u32;
        case VAL_F32: return (double)val.as.as_f32;
        case VAL_F64: return val.as.as_f64;
        default:
            fprintf(stderr, "Runtime error: Cannot convert to float\n");
            exit(1);
    }
}

// Helper: Check if value is truthy
static int value_is_truthy(Value val) {
    if (val.type == VAL_BOOL) {
        return val.as.as_bool;
    } else if (is_integer(val)) {
        return value_to_int(val) != 0;
    } else if (is_float(val)) {
        return value_to_float(val) != 0.0;
    } else if (val.type == VAL_NULL) {
        return 0;
    }
    return 1;  // strings, etc. are truthy
}

// Get the "rank" of a type for promotion rules
static int type_rank(ValueType type) {
    switch (type) {
        case VAL_I8: return 0;
        case VAL_U8: return 1;
        case VAL_I16: return 2;
        case VAL_U16: return 3;
        case VAL_I32: return 4;
        case VAL_U32: return 5;
        case VAL_F32: return 6;
        case VAL_F64: return 7;
        default: return -1;
    }
}

// Determine the result type for binary operations
static ValueType promote_types(ValueType left, ValueType right) {
    // If types are the same, no promotion needed
    if (left == right) return left;
    
    // Any float beats any int
    if (is_float((Value){.type = left})) {
        if (is_float((Value){.type = right})) {
            // Both floats - take the larger
            return (left == VAL_F64 || right == VAL_F64) ? VAL_F64 : VAL_F32;
        }
        // left is float, right is int
        return left;
    }
    if (is_float((Value){.type = right})) {
        // right is float, left is int
        return right;
    }
    
    // Both are integers - promote to higher rank
    int left_rank = type_rank(left);
    int right_rank = type_rank(right);
    
    return (left_rank > right_rank) ? left : right;
}

// Convert a value to a specific ValueType (for promotions during operations)
static Value promote_value(Value val, ValueType target_type) {
    if (val.type == target_type) return val;
    
    switch (target_type) {
        case VAL_I8: return val_i8((int8_t)value_to_int(val));
        case VAL_I16: return val_i16((int16_t)value_to_int(val));
        case VAL_I32: return val_i32(value_to_int(val));
        case VAL_U8: return val_u8((uint8_t)value_to_int(val));
        case VAL_U16: return val_u16((uint16_t)value_to_int(val));
        case VAL_U32: return val_u32((uint32_t)value_to_int(val));
        case VAL_F32:
            if (is_float(val)) {
                return val_f32((float)value_to_float(val));
            } else {
                return val_f32((float)value_to_int(val));
            }
        case VAL_F64:
            if (is_float(val)) {
                return val_f64(value_to_float(val));
            } else {
                return val_f64((double)value_to_int(val));
            }
        default:
            fprintf(stderr, "Runtime error: Cannot promote to type\n");
            exit(1);
    }
}

// ========== STRING OPERATIONS ==========

void string_free(String *str) {
    if (str) {
        free(str->data);
        free(str);
    }
}

String* string_new(const char *cstr) {
    int len = strlen(cstr);
    String *str = malloc(sizeof(String));
    str->length = len;
    str->capacity = len + 1;
    str->data = malloc(str->capacity);
    memcpy(str->data, cstr, len);
    str->data[len] = '\0';
    return str;
}

String* string_copy(String *str) {
    String *copy = malloc(sizeof(String));
    copy->length = str->length;
    copy->capacity = str->capacity;
    copy->data = malloc(copy->capacity);
    memcpy(copy->data, str->data, str->length + 1);
    return copy;
}

String* string_concat(String *a, String *b) {
    int new_len = a->length + b->length;
    String *result = malloc(sizeof(String));
    result->length = new_len;
    result->capacity = new_len + 1;
    result->data = malloc(result->capacity);
    
    memcpy(result->data, a->data, a->length);
    memcpy(result->data + a->length, b->data, b->length);
    result->data[new_len] = '\0';
    
    return result;
}

Value val_string(const char *str) {
    Value v;
    v.type = VAL_STRING;
    v.as.as_string = string_new(str);
    return v;
}

Value val_string_take(char *data, int length, int capacity) {
    Value v;
    v.type = VAL_STRING;
    String *str = malloc(sizeof(String));
    str->data = data;
    str->length = length;
    str->capacity = capacity;
    v.as.as_string = str;
    return v;
}

// ========== BUFFER OPERATIONS ==========

void buffer_free(Buffer *buf) {
    if (buf) {
        free(buf->data);
        free(buf);
    }
}

Value val_buffer(int size) {
    if (size <= 0) {
        fprintf(stderr, "Runtime error: buffer size must be positive\n");
        exit(1);
    }

    Value v;
    v.type = VAL_BUFFER;
    Buffer *buf = malloc(sizeof(Buffer));
    buf->data = malloc(size);
    if (buf->data == NULL) {
        fprintf(stderr, "Runtime error: Failed to allocate buffer\n");
        exit(1);
    }
    buf->length = size;
    buf->capacity = size;
    v.as.as_buffer = buf;
    return v;
}

// ========== VALUE OPERATIONS ==========

Value val_i8(int8_t value) {
    Value v;
    v.type = VAL_I8;
    v.as.as_i8 = value;
    return v;
}

Value val_i16(int16_t value) {
    Value v;
    v.type = VAL_I16;
    v.as.as_i16 = value;
    return v;
}

Value val_i32(int32_t value) {
    Value v;
    v.type = VAL_I32;
    v.as.as_i32 = value;
    return v;
}

Value val_u8(uint8_t value) {
    Value v;
    v.type = VAL_U8;
    v.as.as_u8 = value;
    return v;
}

Value val_u16(uint16_t value) {
    Value v;
    v.type = VAL_U16;
    v.as.as_u16 = value;
    return v;
}

Value val_u32(uint32_t value) {
    Value v;
    v.type = VAL_U32;
    v.as.as_u32 = value;
    return v;
}

Value val_f32(float value) {
    Value v;
    v.type = VAL_F32;
    v.as.as_f32 = value;
    return v;
}

Value val_f64(double value) {
    Value v;
    v.type = VAL_F64;
    v.as.as_f64 = value;
    return v;
}

Value val_int(int value) {
    return val_i32((int32_t)value);
}

Value val_float(double value) {
    return val_f64(value);
}

Value val_bool(int value) {
    Value v;
    v.type = VAL_BOOL;
    v.as.as_bool = value ? 1 : 0;
    return v;
}

Value val_ptr(void *ptr) {
    Value v;
    v.type = VAL_PTR;
    v.as.as_ptr = ptr;
    return v;
}

Value val_null(void) {
    Value v;
    v.type = VAL_NULL;
    return v;
}

void print_value(Value val) {
    switch (val.type) {
        case VAL_I8:
            printf("%d", val.as.as_i8);
            break;
        case VAL_I16:
            printf("%d", val.as.as_i16);
            break;
        case VAL_I32:
            printf("%d", val.as.as_i32);
            break;
        case VAL_U8:
            printf("%u", val.as.as_u8);
            break;
        case VAL_U16:
            printf("%u", val.as.as_u16);
            break;
        case VAL_U32:
            printf("%u", val.as.as_u32);
            break;
        case VAL_F32:
            printf("%g", val.as.as_f32);
            break;
        case VAL_F64:
            printf("%g", val.as.as_f64);
            break;
        case VAL_BOOL:
            printf("%s", val.as.as_bool ? "true" : "false");
            break;
        case VAL_STRING:
            printf("%s", val.as.as_string->data);
            break;
        case VAL_PTR:
            printf("%p", val.as.as_ptr);
            break;
        case VAL_BUFFER:
            printf("<buffer %p length=%d capacity=%d>",
                   val.as.as_buffer->data,
                   val.as.as_buffer->length,
                   val.as.as_buffer->capacity);
            break;
        case VAL_BUILTIN_FN:
            printf("<builtin function>");
            break;
        case VAL_NULL:
            printf("null");
            break;
    }
}

// Helper to convert a value to a target type
static Value convert_to_type(Value value, TypeKind target_type) {
    // Get the source value as the widest type for range checking
    int64_t int_val = 0;
    double float_val = 0.0;
    int is_source_float = 0;
    
    // Extract source value
    if (is_integer(value)) {
        int_val = value_to_int(value);
    } else if (is_float(value)) {
        float_val = value_to_float(value);
        is_source_float = 1;
    } else if (value.type == VAL_BOOL) {
        // Allow bool -> int conversions
        int_val = value.as.as_bool;
    } else if (value.type == VAL_STRING && target_type == TYPE_STRING) {
        return value;  // String to string, ok
    } else if (value.type == VAL_BOOL && target_type == TYPE_BOOL) {
        return value;  // Bool to bool, ok
    } else if (value.type == VAL_NULL && target_type == TYPE_NULL) {
        return value;  // Null to null, ok
    } else {
        fprintf(stderr, "Runtime error: Cannot convert type to target type\n");
        exit(1);
    }
    
    switch (target_type) {
        case TYPE_I8:
            if (is_source_float) {
                int_val = (int64_t)float_val;
            }
            if (int_val < -128 || int_val > 127) {
                fprintf(stderr, "Runtime error: Value %ld out of range for i8 [-128, 127]\n", int_val);
                exit(1);
            }
            return val_i8((int8_t)int_val);
            
        case TYPE_I16:
            if (is_source_float) {
                int_val = (int64_t)float_val;
            }
            if (int_val < -32768 || int_val > 32767) {
                fprintf(stderr, "Runtime error: Value %ld out of range for i16 [-32768, 32767]\n", int_val);
                exit(1);
            }
            return val_i16((int16_t)int_val);
            
        case TYPE_I32:
            if (is_source_float) {
                int_val = (int64_t)float_val;
            }
            if (int_val < -2147483648LL || int_val > 2147483647LL) {
                fprintf(stderr, "Runtime error: Value %ld out of range for i32 [-2147483648, 2147483647]\n", int_val);
                exit(1);
            }
            return val_i32((int32_t)int_val);
            
        case TYPE_U8:
            if (is_source_float) {
                int_val = (int64_t)float_val;
            }
            if (int_val < 0 || int_val > 255) {
                fprintf(stderr, "Runtime error: Value %ld out of range for u8 [0, 255]\n", int_val);
                exit(1);
            }
            return val_u8((uint8_t)int_val);
            
        case TYPE_U16:
            if (is_source_float) {
                int_val = (int64_t)float_val;
            }
            if (int_val < 0 || int_val > 65535) {
                fprintf(stderr, "Runtime error: Value %ld out of range for u16 [0, 65535]\n", int_val);
                exit(1);
            }
            return val_u16((uint16_t)int_val);
            
        case TYPE_U32:
            if (is_source_float) {
                int_val = (int64_t)float_val;
            }
            if (int_val < 0 || int_val > 4294967295LL) {
                fprintf(stderr, "Runtime error: Value %ld out of range for u32 [0, 4294967295]\n", int_val);
                exit(1);
            }
            return val_u32((uint32_t)int_val);
            
        case TYPE_F32:
            if (is_source_float) {
                return val_f32((float)float_val);
            } else {
                return val_f32((float)int_val);
            }
            
        case TYPE_F64:
            if (is_source_float) {
                return val_f64(float_val);
            } else {
                return val_f64((double)int_val);
            }
            
        case TYPE_BOOL:
            if (value.type == VAL_BOOL) {
                return value;
            }
            fprintf(stderr, "Runtime error: Cannot convert to bool\n");
            exit(1);
            
        case TYPE_STRING:
            if (value.type == VAL_STRING) {
                return value;
            }
            fprintf(stderr, "Runtime error: Cannot convert to string\n");
            exit(1);

        case TYPE_PTR:
            if (value.type == VAL_PTR) {
                return value;
            }
            fprintf(stderr, "Runtime error: Cannot convert to ptr\n");
            exit(1);

        case TYPE_BUFFER:
            if (value.type == VAL_BUFFER) {
                return value;
            }
            fprintf(stderr, "Runtime error: Cannot convert to buffer\n");
            exit(1);

        case TYPE_NULL:
            return val_null();

        case TYPE_INFER:
            return value;  // No conversion needed
    }
    
    fprintf(stderr, "Runtime error: Unknown type conversion\n");
    exit(1);
}

// ========== ENVIRONMENT ==========

Environment* env_new(Environment *parent) {
    Environment *env = malloc(sizeof(Environment));
    env->capacity = 16;
    env->count = 0;
    env->names = malloc(sizeof(char*) * env->capacity);
    env->values = malloc(sizeof(Value) * env->capacity);
    env->parent = parent;
    return env;
}

void env_free(Environment *env) {
    for (int i = 0; i < env->count; i++) {
        free(env->names[i]);
    }
    free(env->names);
    free(env->values);
    free(env);
}

static void env_grow(Environment *env) {
    env->capacity *= 2;
    env->names = realloc(env->names, sizeof(char*) * env->capacity);
    env->values = realloc(env->values, sizeof(Value) * env->capacity);
}

void env_set(Environment *env, const char *name, Value value) {
    // Check if variable already exists - update it
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->names[i], name) == 0) {
            env->values[i] = value;
            return;
        }
    }
    
    // New variable
    if (env->count >= env->capacity) {
        env_grow(env);
    }
    
    env->names[env->count] = strdup(name);
    env->values[env->count] = value;
    env->count++;
}

Value env_get(Environment *env, const char *name) {
    // Search current scope
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->names[i], name) == 0) {
            return env->values[i];
        }
    }
    
    // Search parent scope
    if (env->parent != NULL) {
        return env_get(env->parent, name);
    }
    
    // Variable not found
    fprintf(stderr, "Runtime error: Undefined variable '%s'\n", name);
    exit(1);
}

// ========== EXPRESSION EVALUATION ==========

Value eval_expr(Expr *expr, Environment *env) {
    switch (expr->type) {
        case EXPR_NUMBER:
            if (expr->as.number.is_float) {
                return val_float(expr->as.number.float_value);
            } else {
                return val_int(expr->as.number.int_value);
            }
            break;

        case EXPR_BOOL:
            return val_bool(expr->as.boolean);

        case EXPR_STRING:
            return val_string(expr->as.string);

        case EXPR_UNARY: {
            Value operand = eval_expr(expr->as.unary.operand, env);
            
            switch (expr->as.unary.op) {
                case UNARY_NOT:
                    return val_bool(!value_is_truthy(operand));
                    
                case UNARY_NEGATE:
                    if (is_float(operand)) {
                        return val_f64(-value_to_float(operand));
                    } else if (is_integer(operand)) {
                        return val_i32(-value_to_int(operand));
                    }
                    fprintf(stderr, "Runtime error: Cannot negate non-numeric value\n");
                    exit(1);
            }
            break;
        }
            
        case EXPR_IDENT:
            return env_get(env, expr->as.ident);

        case EXPR_ASSIGN: {
            Value value = eval_expr(expr->as.assign.value, env);
            env_set(env, expr->as.assign.name, value);
            return value;
        }
           
        case EXPR_BINARY: {
            // Handle && and || with short-circuit evaluation
            if (expr->as.binary.op == OP_AND) {
                Value left = eval_expr(expr->as.binary.left, env);
                if (!value_is_truthy(left)) return val_bool(0);
                
                Value right = eval_expr(expr->as.binary.right, env);
                return val_bool(value_is_truthy(right));
            }
            
            if (expr->as.binary.op == OP_OR) {
                Value left = eval_expr(expr->as.binary.left, env);
                if (value_is_truthy(left)) return val_bool(1);
                
                Value right = eval_expr(expr->as.binary.right, env);
                return val_bool(value_is_truthy(right));
            }
            
            // Evaluate both operands
            Value left = eval_expr(expr->as.binary.left, env);
            Value right = eval_expr(expr->as.binary.right, env);
            
            // String concatenation
            if (expr->as.binary.op == OP_ADD && left.type == VAL_STRING && right.type == VAL_STRING) {
                String *result = string_concat(left.as.as_string, right.as.as_string);
                return (Value){ .type = VAL_STRING, .as.as_string = result };
            }

            // Pointer arithmetic
            if (left.type == VAL_PTR && is_integer(right)) {
                if (expr->as.binary.op == OP_ADD) {
                    void *ptr = left.as.as_ptr;
                    int32_t offset = value_to_int(right);
                    return val_ptr((char *)ptr + offset);
                } else if (expr->as.binary.op == OP_SUB) {
                    void *ptr = left.as.as_ptr;
                    int32_t offset = value_to_int(right);
                    return val_ptr((char *)ptr - offset);
                }
            }

            if (is_integer(left) && right.type == VAL_PTR) {
                if (expr->as.binary.op == OP_ADD) {
                    int32_t offset = value_to_int(left);
                    void *ptr = right.as.as_ptr;
                    return val_ptr((char *)ptr + offset);
                }
            }
            
            // Numeric operations
            if (!is_numeric(left) || !is_numeric(right)) {
                fprintf(stderr, "Runtime error: Binary operation requires numeric operands\n");
                exit(1);
            }
            
            // Determine result type and promote operands
            ValueType result_type = promote_types(left.type, right.type);
            left = promote_value(left, result_type);
            right = promote_value(right, result_type);
            
            // Perform operation based on result type
            if (is_float(left)) {
                // Float operation
                double l = value_to_float(left);
                double r = value_to_float(right);
                
                switch (expr->as.binary.op) {
                    case OP_ADD: 
                        return (result_type == VAL_F32) ? val_f32((float)(l + r)) : val_f64(l + r);
                    case OP_SUB:
                        return (result_type == VAL_F32) ? val_f32((float)(l - r)) : val_f64(l - r);
                    case OP_MUL:
                        return (result_type == VAL_F32) ? val_f32((float)(l * r)) : val_f64(l * r);
                    case OP_DIV:
                        if (r == 0.0) {
                            fprintf(stderr, "Runtime error: Division by zero\n");
                            exit(1);
                        }
                        return (result_type == VAL_F32) ? val_f32((float)(l / r)) : val_f64(l / r);
                    case OP_EQUAL: return val_bool(l == r);
                    case OP_NOT_EQUAL: return val_bool(l != r);
                    case OP_LESS: return val_bool(l < r);
                    case OP_LESS_EQUAL: return val_bool(l <= r);
                    case OP_GREATER: return val_bool(l > r);
                    case OP_GREATER_EQUAL: return val_bool(l >= r);
                    default: break;
                }
            } else {
                // Integer operation - use the promoted type
                int64_t l = value_to_int(left);
                int64_t r = value_to_int(right);
                
                switch (expr->as.binary.op) {
                    case OP_ADD:
                        return promote_value(val_i32((int32_t)(l + r)), result_type);
                    case OP_SUB:
                        return promote_value(val_i32((int32_t)(l - r)), result_type);
                    case OP_MUL:
                        return promote_value(val_i32((int32_t)(l * r)), result_type);
                    case OP_DIV:
                        if (r == 0) {
                            fprintf(stderr, "Runtime error: Division by zero\n");
                            exit(1);
                        }
                        return promote_value(val_i32((int32_t)(l / r)), result_type);
                    case OP_EQUAL: return val_bool(l == r);
                    case OP_NOT_EQUAL: return val_bool(l != r);
                    case OP_LESS: return val_bool(l < r);
                    case OP_LESS_EQUAL: return val_bool(l <= r);
                    case OP_GREATER: return val_bool(l > r);
                    case OP_GREATER_EQUAL: return val_bool(l >= r);
                    default: break;
                }
            }
            
            fprintf(stderr, "Runtime error: Unknown binary operator\n");
            exit(1);
        }
            
        case EXPR_CALL: {
            // Look up the function
            Value func = env_get(env, expr->as.call.name);
            
            // Evaluate arguments
            Value *args = NULL;
            if (expr->as.call.num_args > 0) {
                args = malloc(sizeof(Value) * expr->as.call.num_args);
                for (int i = 0; i < expr->as.call.num_args; i++) {
                    args[i] = eval_expr(expr->as.call.args[i], env);
                }
            }
            
            Value result;
            
            if (func.type == VAL_BUILTIN_FN) {
                // Call builtin function
                BuiltinFn fn = func.as.as_builtin_fn;
                result = fn(args, expr->as.call.num_args);
            } else {
                // Check for special case: print (we haven't removed this yet)
                if (strcmp(expr->as.call.name, "print") == 0) {
                    if (expr->as.call.num_args != 1) {
                        fprintf(stderr, "Runtime error: print() expects 1 argument\n");
                        exit(1);
                    }
                    print_value(args[0]);
                    printf("\n");
                    result = val_null();
                } else {
                    fprintf(stderr, "Runtime error: Unknown function '%s'\n", expr->as.call.name);
                    exit(1);
                }
            }
            
            if (args) free(args);
            return result;
        }

        case EXPR_GET_PROPERTY: {
            Value object = eval_expr(expr->as.get_property.object, env);
            const char *property = expr->as.get_property.property;

            if (object.type == VAL_STRING) {
                if (strcmp(property, "length") == 0) {
                    return val_int(object.as.as_string->length);
                }
                fprintf(stderr, "Runtime error: Unknown property '%s' for string\n", property);
                exit(1);
            } else if (object.type == VAL_BUFFER) {
                if (strcmp(property, "length") == 0) {
                    return val_int(object.as.as_buffer->length);
                } else if (strcmp(property, "capacity") == 0) {
                    return val_int(object.as.as_buffer->capacity);
                }
                fprintf(stderr, "Runtime error: Unknown property '%s' for buffer\n", property);
                exit(1);
            } else {
                fprintf(stderr, "Runtime error: Only strings and buffers have properties\n");
                exit(1);
            }
        }
        
        case EXPR_INDEX: {
            Value object = eval_expr(expr->as.index.object, env);
            Value index_val = eval_expr(expr->as.index.index, env);

            if (!is_integer(index_val)) {
                fprintf(stderr, "Runtime error: Index must be an integer\n");
                exit(1);
            }

            int32_t index = value_to_int(index_val);

            if (object.type == VAL_STRING) {
                String *str = object.as.as_string;

                if (index < 0 || index >= str->length) {
                    fprintf(stderr, "Runtime error: String index %d out of bounds (length %d)\n",
                            index, str->length);
                    exit(1);
                }

                // Return the byte as an integer (u8/char)
                return val_u8((unsigned char)str->data[index]);
            } else if (object.type == VAL_BUFFER) {
                Buffer *buf = object.as.as_buffer;

                if (index < 0 || index >= buf->length) {
                    fprintf(stderr, "Runtime error: Buffer index %d out of bounds (length %d)\n",
                            index, buf->length);
                    exit(1);
                }

                // Return the byte as an integer (u8)
                return val_u8(((unsigned char *)buf->data)[index]);
            } else {
                fprintf(stderr, "Runtime error: Only strings and buffers can be indexed\n");
                exit(1);
            }
        }
        
        case EXPR_INDEX_ASSIGN: {
            Value object = eval_expr(expr->as.index_assign.object, env);
            Value index_val = eval_expr(expr->as.index_assign.index, env);
            Value value = eval_expr(expr->as.index_assign.value, env);

            if (!is_integer(index_val)) {
                fprintf(stderr, "Runtime error: Index must be an integer\n");
                exit(1);
            }

            if (!is_integer(value)) {
                fprintf(stderr, "Runtime error: Index value must be an integer (byte)\n");
                exit(1);
            }

            int32_t index = value_to_int(index_val);

            if (object.type == VAL_STRING) {
                String *str = object.as.as_string;

                if (index < 0 || index >= str->length) {
                    fprintf(stderr, "Runtime error: String index %d out of bounds (length %d)\n",
                            index, str->length);
                    exit(1);
                }

                // Strings are mutable - set the byte
                str->data[index] = (char)value_to_int(value);
                return value;
            } else if (object.type == VAL_BUFFER) {
                Buffer *buf = object.as.as_buffer;

                if (index < 0 || index >= buf->length) {
                    fprintf(stderr, "Runtime error: Buffer index %d out of bounds (length %d)\n",
                            index, buf->length);
                    exit(1);
                }

                // Buffers are mutable - set the byte
                ((unsigned char *)buf->data)[index] = (unsigned char)value_to_int(value);
                return value;
            } else {
                fprintf(stderr, "Runtime error: Only strings and buffers can be indexed\n");
                exit(1);
            }
        }
    }
    
    return val_null();
}

// ========== STATEMENT EVALUATION ==========

void eval_stmt(Stmt *stmt, Environment *env) {
    switch (stmt->type) {
        case STMT_LET: {
            Value value = eval_expr(stmt->as.let.value, env);
            // If there's a type annotation, convert/check the value
            if (stmt->as.let.type_annotation != NULL) {
                value = convert_to_type(value, stmt->as.let.type_annotation->kind);
            }
            env_set(env, stmt->as.let.name, value);
            break;
        }
            
        case STMT_EXPR: {
            eval_expr(stmt->as.expr, env);
            break;
        }
            
        case STMT_IF: {
            Value condition = eval_expr(stmt->as.if_stmt.condition, env);
            
            if (value_is_truthy(condition)) {
                eval_stmt(stmt->as.if_stmt.then_branch, env);
            } else if (stmt->as.if_stmt.else_branch != NULL) {
                eval_stmt(stmt->as.if_stmt.else_branch, env);
            }
            break;
        }
            
        case STMT_WHILE: {
            for (;;) {
                Value condition = eval_expr(stmt->as.while_stmt.condition, env);
                
                if (!value_is_truthy(condition)) break;
                
                eval_stmt(stmt->as.while_stmt.body, env);
            }
            break;
        }
            
        case STMT_BLOCK: {
            for (int i = 0; i < stmt->as.block.count; i++) {
                eval_stmt(stmt->as.block.statements[i], env);
            }
            break;
        }
    }
}

void eval_program(Stmt **stmts, int count, Environment *env) {
    for (int i = 0; i < count; i++) {
        eval_stmt(stmts[i], env);
    }
}

// ========== BUILTIN FUNCTIONS ==========

static Value builtin_print(Value *args, int num_args) {
    if (num_args != 1) {
        fprintf(stderr, "Runtime error: print() expects 1 argument\n");
        exit(1);
    }

    print_value(args[0]);
    printf("\n");
    return val_null();
}

static Value builtin_alloc(Value *args, int num_args) {
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

static Value builtin_free(Value *args, int num_args) {
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

static Value builtin_memset(Value *args, int num_args) {
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

static Value builtin_memcpy(Value *args, int num_args) {
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

static Value builtin_sizeof(Value *args, int num_args) {
    (void)args;
    (void)num_args;

    if (num_args != 1) {
        fprintf(stderr, "Runtime error: sizeof() expects 1 argument (type)\n");
        exit(1);
    }

    // For now, assume the argument is a special type identifier
    // We'll need to extend this later to handle type values
    // For simplicity, we'll just return sizes for common types

    fprintf(stderr, "Runtime error: sizeof() not fully implemented yet\n");
    exit(1);
}

static Value builtin_buffer(Value *args, int num_args) {
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

// Structure to hold builtin function info
typedef struct {
    const char *name;
    BuiltinFn fn;
} BuiltinInfo;

static BuiltinInfo builtins[] = {
    {"print", builtin_print},
    {"alloc", builtin_alloc},
    {"free", builtin_free},
    {"memset", builtin_memset},
    {"memcpy", builtin_memcpy},
    {"sizeof", builtin_sizeof},
    {"buffer", builtin_buffer},
    {NULL, NULL}  // Sentinel
};

Value val_builtin_fn(BuiltinFn fn) {
    Value v;
    v.type = VAL_BUILTIN_FN;
    v.as.as_builtin_fn = fn;
    return v;
}

void register_builtins(Environment *env) {
    for (int i = 0; builtins[i].name != NULL; i++) {
        env_set(env, builtins[i].name, val_builtin_fn(builtins[i].fn));
    }
}