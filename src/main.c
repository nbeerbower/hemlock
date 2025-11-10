#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "interpreter.h"

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

static void run_source(const char *source) {
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
    register_builtins(env);
    eval_program(statements, stmt_count, env);
    
    // Cleanup
    env_free(env);
    for (int i = 0; i < stmt_count; i++) {
        stmt_free(statements[i]);
    }
    free(statements);
}

static void run_file(const char *path) {
    char *source = read_file(path);
    if (source == NULL) {
        exit(1);
    }
    
    run_source(source);
    free(source);
}

static void run_repl(void) {
    char line[1024];
    Environment *env = env_new(NULL);
    register_builtins(env);
    
    printf("Hemlock v0.1 REPL\n");
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
            eval_stmt(statements[i], env);
        }
        
        // Cleanup
        for (int i = 0; i < stmt_count; i++) {
            stmt_free(statements[i]);
        }
        free(statements);
    }
    
    env_free(env);
}

static void print_usage(const char *program) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s [file.hml]    Run a hemlock file\n", program);
    fprintf(stderr, "  %s               Start REPL\n", program);
}

int main(int argc, char **argv) {
    if (argc == 1) {
        // No arguments - start REPL
        run_repl();
        return 0;
    }
    
    if (argc == 2) {
        // One argument - run file
        run_file(argv[1]);
        exit(0);  // Explicitly exit with success code after file execution
    }
    
    // Too many arguments
    print_usage(argv[0]);
    return 1;
}