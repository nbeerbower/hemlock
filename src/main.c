#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "interpreter.h"
#include "module.h"
#include "interpreter/internal.h"
#include "lsp/lsp.h"
#include "ast_serialize.h"

#define HEMLOCK_VERSION "1.0.0"
#define HEMLOCK_BUILD_DATE __DATE__

// FFI functions (from interpreter/ffi.c)
extern void ffi_init(void);
extern void ffi_cleanup(void);

// Read entire file into a string (caller must free)
static char* read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open file '%s'\n", path);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    
    // Allocate buffer
    char *buffer = malloc(size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Error: Could not allocate memory for file\n");
        fclose(file);
        return NULL;
    }
    
    // Read file
    size_t bytes_read = fread(buffer, 1, size, file);
    if (bytes_read < (size_t)size) {
        fprintf(stderr, "Error: Could not read file\n");
        free(buffer);
        fclose(file);
        return NULL;
    }
    
    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

static void run_source(const char *source, int argc, char **argv) {
    // Parse
    Lexer lexer;
    lexer_init(&lexer, source);

    Parser parser;
    parser_init(&parser, &lexer);

    int stmt_count;
    Stmt **statements = parse_program(&parser, &stmt_count);

    if (parser.had_error) {
        fprintf(stderr, "Parse failed!\n");
        exit(1);
    }

    // Interpret
    Environment *env = env_new(NULL);

    // Create execution context
    ExecutionContext *ctx = exec_context_new();

    register_builtins(env, argc, argv, ctx);

    eval_program(statements, stmt_count, env, ctx);

    // Cleanup
    exec_context_free(ctx);
    env_break_cycles(env);  // Break circular references before release
    env_release(env);
    clear_manually_freed_pointers();  // Clear after env is fully freed
    for (int i = 0; i < stmt_count; i++) {
        stmt_free(statements[i]);
    }
    free(statements);
}

// Check if source contains import or export statements
static int has_modules(const char *source) {
    // Simple check: look for "import " or "export " keywords
    // This is a heuristic - not perfect but good enough
    const char *p = source;
    while (*p) {
        // Skip whitespace
        while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;

        // Check for import or export at start of line or after whitespace
        if (strncmp(p, "import ", 7) == 0 || strncmp(p, "import{", 7) == 0 ||
            strncmp(p, "export ", 7) == 0 || strncmp(p, "export{", 7) == 0) {
            return 1;
        }

        // Skip to next line
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }
    return 0;
}

static void run_file(const char *path, int argc, char **argv) {
    char *source = read_file(path);
    if (source == NULL) {
        exit(1);
    }

    // Initialize FFI
    ffi_init();

    // Set current source file for stack traces
    set_current_source_file(path);

    // Check if file uses modules
    if (has_modules(source)) {
        // Use module system
        ExecutionContext *ctx = exec_context_new();

        // Need to set up builtins in a global environment first
        Environment *global_env = env_new(NULL);
        register_builtins(global_env, argc, argv, ctx);

        // Execute with module system
        int result = execute_file_with_modules(path, global_env, argc, argv, ctx);

        env_break_cycles(global_env);  // Break circular references before release
        env_release(global_env);
        clear_manually_freed_pointers();  // Clear after env is fully freed
        exec_context_free(ctx);
        free(source);

        // Cleanup FFI and source file tracking
        ffi_cleanup();
        set_current_source_file(NULL);

        if (result != 0) {
            exit(1);
        }
    } else {
        // Use traditional execution
        run_source(source, argc, argv);
        free(source);

        // Cleanup FFI and source file tracking
        ffi_cleanup();
        set_current_source_file(NULL);
    }
}

// Compile .hml file to .hmlc binary AST format
static int compile_file(const char *input_path, const char *output_path, int debug_info) {
    char *source = read_file(input_path);
    if (source == NULL) {
        return 1;
    }

    // Parse
    Lexer lexer;
    lexer_init(&lexer, source);

    Parser parser;
    parser_init(&parser, &lexer);

    int stmt_count;
    Stmt **statements = parse_program(&parser, &stmt_count);

    if (parser.had_error) {
        fprintf(stderr, "Compilation failed: parse errors in '%s'\n", input_path);
        free(source);
        return 1;
    }

    // Determine output path
    char *final_output = NULL;
    if (output_path == NULL) {
        // Default: replace .hml with .hmlc or append .hmlc
        size_t len = strlen(input_path);
        final_output = malloc(len + 5);  // Room for ".hmlc\0"
        strcpy(final_output, input_path);

        // Check for .hml extension
        if (len > 4 && strcmp(input_path + len - 4, ".hml") == 0) {
            strcpy(final_output + len - 4, ".hmlc");
        } else {
            strcat(final_output, ".hmlc");
        }
    } else {
        final_output = strdup(output_path);
    }

    // Serialize
    uint16_t flags = 0;
    if (debug_info) {
        flags |= HMLC_FLAG_DEBUG;
    }

    int result = ast_serialize_to_file(final_output, statements, stmt_count, flags);

    if (result == 0) {
        // Get file size for reporting
        FILE *f = fopen(final_output, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fclose(f);
            printf("Compiled '%s' -> '%s' (%ld bytes)\n", input_path, final_output, size);
        } else {
            printf("Compiled '%s' -> '%s'\n", input_path, final_output);
        }
    } else {
        fprintf(stderr, "Failed to write compiled output to '%s'\n", final_output);
    }

    // Cleanup
    free(final_output);
    free(source);
    for (int i = 0; i < stmt_count; i++) {
        stmt_free(statements[i]);
    }
    free(statements);

    return result;
}

// Run a .hmlc compiled file
static void run_hmlc_file(const char *path, int argc, char **argv) {
    // Deserialize AST from file
    int stmt_count;
    Stmt **statements = ast_deserialize_from_file(path, &stmt_count);
    if (statements == NULL) {
        fprintf(stderr, "Failed to load compiled file '%s'\n", path);
        exit(1);
    }

    // Initialize FFI
    ffi_init();

    // Set current source file for stack traces
    set_current_source_file(path);

    // Create execution environment
    Environment *env = env_new(NULL);
    ExecutionContext *ctx = exec_context_new();
    register_builtins(env, argc, argv, ctx);

    // Execute
    eval_program(statements, stmt_count, env, ctx);

    // Cleanup
    exec_context_free(ctx);
    env_break_cycles(env);
    env_release(env);
    clear_manually_freed_pointers();

    for (int i = 0; i < stmt_count; i++) {
        stmt_free(statements[i]);
    }
    free(statements);

    ffi_cleanup();
    set_current_source_file(NULL);
}

// Check if file has .hmlc extension
static int is_hmlc_extension(const char *path) {
    size_t len = strlen(path);
    return len > 5 && strcmp(path + len - 5, ".hmlc") == 0;
}

static void run_repl(void) {
    char line[1024];
    Environment *env = env_new(NULL);

    // Create execution context for REPL (persists across lines)
    ExecutionContext *ctx = exec_context_new();

    // Initialize FFI
    ffi_init();

    register_builtins(env, 0, NULL, ctx);

    printf("Hemlock v%s REPL\n", HEMLOCK_VERSION);
    printf("Type 'exit' to quit\n\n");

    for (;;) {
        printf(">>> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        // Remove newline
        line[strcspn(line, "\n")] = 0;

        // Check for exit
        if (strcmp(line, "exit") == 0) {
            break;
        }

        // Skip empty lines
        if (strlen(line) == 0) {
            continue;
        }

        // Parse and execute
        Lexer lexer;
        lexer_init(&lexer, line);

        Parser parser;
        parser_init(&parser, &lexer);

        int stmt_count;
        Stmt **statements = parse_program(&parser, &stmt_count);

        if (parser.had_error) {
            continue;
        }

        // Execute
        for (int i = 0; i < stmt_count; i++) {
            eval_stmt(statements[i], env, ctx);
        }

        // Cleanup
        for (int i = 0; i < stmt_count; i++) {
            stmt_free(statements[i]);
        }
        free(statements);
    }

    // Cleanup FFI
    ffi_cleanup();

    exec_context_free(ctx);
    env_break_cycles(env);  // Break circular references before release
    env_release(env);
    clear_manually_freed_pointers();  // Clear after env is fully freed
}

static void print_version(void) {
    printf("Hemlock version %s (built %s)\n", HEMLOCK_VERSION, HEMLOCK_BUILD_DATE);
    printf("A small, unsafe language for writing unsafe things safely.\n");
}

static void print_help(const char *program) {
    printf("Hemlock - A systems scripting language\n\n");
    printf("USAGE:\n");
    printf("    %s [OPTIONS] [FILE] [ARGS...]\n", program);
    printf("    %s --compile FILE [-o OUTPUT] [--debug]\n", program);
    printf("    %s lsp [--stdio | --tcp PORT]\n\n", program);
    printf("ARGUMENTS:\n");
    printf("    <FILE>       Hemlock script file to execute (.hml or .hmlc)\n");
    printf("    <ARGS>...    Arguments passed to the script (available in 'args' array)\n\n");
    printf("SUBCOMMANDS:\n");
    printf("    lsp          Start Language Server Protocol server\n");
    printf("        --stdio      Use stdio transport (default)\n");
    printf("        --tcp PORT   Use TCP transport on specified port\n\n");
    printf("OPTIONS:\n");
    printf("    -h, --help           Display this help message\n");
    printf("    -v, --version        Display version information\n");
    printf("    -i, --interactive    Start REPL after executing file\n");
    printf("    -c, --command <CODE> Execute code string directly\n");
    printf("    --compile <FILE>     Compile .hml to binary AST (.hmlc)\n");
    printf("    -o, --output <FILE>  Output path for compiled file\n");
    printf("    --debug              Include line numbers in compiled output\n\n");
    printf("EXAMPLES:\n");
    printf("    %s                     # Start interactive REPL\n", program);
    printf("    %s script.hml          # Run script.hml\n", program);
    printf("    %s script.hmlc         # Run compiled script\n", program);
    printf("    %s script.hml arg1 arg2    # Run script with arguments\n", program);
    printf("    %s -c 'print(\"Hello\");'    # Execute code string\n", program);
    printf("    %s -i script.hml       # Run script then start REPL\n", program);
    printf("    %s --compile script.hml    # Compile to script.hmlc\n", program);
    printf("    %s --compile src.hml -o out.hmlc --debug\n", program);
    printf("    %s lsp                 # Start LSP server (stdio)\n", program);
    printf("    %s lsp --tcp 6969      # Start LSP server (TCP)\n\n", program);
    printf("For more information, visit: https://github.com/nbeerbower/hemlock\n");
}

// Run LSP server
static int run_lsp(int argc, char **argv) {
    int use_tcp = 0;
    int tcp_port = 6969;

    // Parse LSP-specific options
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--stdio") == 0) {
            use_tcp = 0;
        } else if (strcmp(argv[i], "--tcp") == 0) {
            use_tcp = 1;
            if (i + 1 < argc) {
                tcp_port = atoi(argv[i + 1]);
                i++;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Hemlock LSP Server\n\n");
            printf("USAGE:\n");
            printf("    hemlock lsp [OPTIONS]\n\n");
            printf("OPTIONS:\n");
            printf("    --stdio          Use stdio transport (default)\n");
            printf("    --tcp PORT       Use TCP transport on specified port\n");
            printf("    -h, --help       Display this help message\n");
            return 0;
        }
    }

    LSPServer *server = lsp_server_create();

    int result;
    if (use_tcp) {
        result = lsp_server_run_tcp(server, tcp_port);
    } else {
        result = lsp_server_run_stdio(server);
    }

    lsp_server_free(server);
    return result;
}

int main(int argc, char **argv) {
    int interactive_mode = 0;
    int compile_mode = 0;
    int compile_debug = 0;
    const char *file_to_run = NULL;
    const char *file_to_compile = NULL;
    const char *output_path = NULL;
    const char *command_to_run = NULL;
    int first_script_arg = 0;  // Index of first argument to pass to script

    // Check for LSP subcommand first
    if (argc >= 2 && strcmp(argv[1], "lsp") == 0) {
        return run_lsp(argc, argv);
    }

    // Parse command-line flags
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            interactive_mode = 1;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--command") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -c/--command requires a code argument\n");
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                return 1;
            }
            command_to_run = argv[i + 1];
            i++;  // Skip the code argument
        } else if (strcmp(argv[i], "--compile") == 0) {
            compile_mode = 1;
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --compile requires a file argument\n");
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                return 1;
            }
            file_to_compile = argv[i + 1];
            i++;  // Skip the file argument
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -o/--output requires a file argument\n");
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                return 1;
            }
            output_path = argv[i + 1];
            i++;  // Skip the output path
        } else if (strcmp(argv[i], "--debug") == 0) {
            compile_debug = 1;
        } else if (argv[i][0] == '-') {
            // Unknown flag
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
            return 1;
        } else {
            // First non-flag argument is the file to run
            file_to_run = argv[i];
            first_script_arg = i;
            break;  // Everything after this is passed to the script
        }
    }

    // Execute based on what was specified

    // Handle compile mode
    if (compile_mode) {
        if (file_to_compile == NULL) {
            fprintf(stderr, "Error: No input file specified for compilation\n");
            return 1;
        }
        int result = compile_file(file_to_compile, output_path, compile_debug);
        return result;
    }

    if (command_to_run != NULL) {
        // Execute code string
        ffi_init();
        run_source(command_to_run, 0, NULL);
        ffi_cleanup();

        if (interactive_mode) {
            run_repl();
        }

        // Cleanup type registries before exit
        cleanup_object_types();
        cleanup_enum_types();
        return 0;
    }

    if (file_to_run != NULL) {
        // Run file with arguments
        int script_argc = argc - first_script_arg;
        char **script_argv = &argv[first_script_arg];

        // Check if it's a compiled .hmlc file
        if (is_hmlc_extension(file_to_run) || is_hmlc_file(file_to_run)) {
            run_hmlc_file(file_to_run, script_argc, script_argv);
        } else {
            run_file(file_to_run, script_argc, script_argv);
        }

        if (interactive_mode) {
            run_repl();
        }

        // Cleanup type registries before exit
        cleanup_object_types();
        cleanup_enum_types();
        return 0;
    }

    // No file or command specified - start REPL
    run_repl();

    // Cleanup type registries before exit
    cleanup_object_types();
    cleanup_enum_types();
    return 0;
}