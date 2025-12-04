/**
 * Hemlock Bundler - Module Resolution and Bundling
 *
 * This module provides functionality to:
 * 1. Recursively resolve all imports from an entry point
 * 2. Flatten multiple modules into a single AST
 * 3. Handle symbol namespacing to avoid collisions
 * 4. Output a unified bundle ready for serialization or compilation
 */

#ifndef HEMLOCK_BUNDLER_H
#define HEMLOCK_BUNDLER_H

#include "../include/ast.h"
#include "../include/module.h"
#include <stdint.h>

// ========== BUNDLE STRUCTURES ==========

// Represents a resolved module in the bundle
typedef struct BundledModule {
    char *absolute_path;         // Resolved absolute path
    char *module_id;             // Unique ID for namespacing (e.g., "mod_0", "mod_1")
    Stmt **statements;           // Parsed AST
    int num_statements;
    char **export_names;         // Names exported by this module
    int num_exports;
    int is_entry;                // 1 if this is the entry point module
    int is_flattened;            // 1 if already flattened into output
} BundledModule;

// Represents the complete bundle
typedef struct Bundle {
    BundledModule **modules;     // Array of all resolved modules
    int num_modules;
    int capacity;

    char *entry_path;            // Absolute path of entry point
    char *stdlib_path;           // Path to stdlib directory

    // Flattened output
    Stmt **statements;           // Unified statement list
    int num_statements;
    int stmt_capacity;
} Bundle;

// Bundle options
typedef struct BundleOptions {
    int include_stdlib;          // 1 to include stdlib modules (default: 1)
    int tree_shake;              // 1 to remove unused exports (default: 0, not yet implemented)
    int namespace_symbols;       // 1 to prefix symbols with module ID (default: 1)
    int verbose;                 // 1 to print progress (default: 0)
} BundleOptions;

// ========== PUBLIC API ==========

/**
 * Create default bundle options
 */
BundleOptions bundle_options_default(void);

/**
 * Create a new bundle from an entry point file
 *
 * @param entry_path  Path to the entry point .hml file
 * @param options     Bundle options (or NULL for defaults)
 * @return            Bundle struct (caller must free with bundle_free)
 *                    Returns NULL on error
 */
Bundle* bundle_create(const char *entry_path, const BundleOptions *options);

/**
 * Flatten the bundle into a single unified AST
 *
 * This resolves all imports and merges all modules into bundle->statements.
 * After calling this, you can serialize the bundle or pass it to codegen.
 *
 * @param bundle      The bundle to flatten
 * @return            0 on success, -1 on error
 */
int bundle_flatten(Bundle *bundle);

/**
 * Get the flattened statements from a bundle
 *
 * @param bundle      The bundle (must have been flattened)
 * @param out_count   Output: number of statements
 * @return            Array of statements (owned by bundle, do not free individually)
 */
Stmt** bundle_get_statements(Bundle *bundle, int *out_count);

/**
 * Write bundle to a .hmlc file
 *
 * @param bundle      The bundle to write
 * @param output_path Output file path
 * @param flags       Serialization flags (HMLC_FLAG_*)
 * @return            0 on success, -1 on error
 */
int bundle_write_hmlc(Bundle *bundle, const char *output_path, uint16_t flags);

/**
 * Write bundle to a compressed .hmlb file
 *
 * @param bundle      The bundle to write
 * @param output_path Output file path
 * @return            0 on success, -1 on error
 */
int bundle_write_compressed(Bundle *bundle, const char *output_path);

/**
 * Free a bundle and all its resources
 */
void bundle_free(Bundle *bundle);

/**
 * Get a module from the bundle by path
 *
 * @param bundle      The bundle to search
 * @param path        Absolute path to look up
 * @return            BundledModule or NULL if not found
 */
BundledModule* bundle_get_module(Bundle *bundle, const char *path);

/**
 * Print bundle summary (for debugging)
 */
void bundle_print_summary(Bundle *bundle);

#endif // HEMLOCK_BUNDLER_H
