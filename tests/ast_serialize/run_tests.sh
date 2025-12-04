#!/bin/bash

# AST Serialization Tests
# Tests that .hmlc files produce identical output to .hml files

set -e

HEMLOCK="./hemlock"
TEST_DIR="tests/ast_serialize"
TEMP_DIR="/tmp/hemlock_ast_tests"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Create temp directory
mkdir -p "$TEMP_DIR"

PASSED=0
FAILED=0

run_test() {
    local test_file="$1"
    local test_name=$(basename "$test_file" .hml)
    local hmlc_file="$TEMP_DIR/${test_name}.hmlc"
    local output_hml="$TEMP_DIR/${test_name}_hml.out"
    local output_hmlc="$TEMP_DIR/${test_name}_hmlc.out"

    echo -n "Testing $test_name... "

    # Run original .hml file
    if ! $HEMLOCK "$test_file" > "$output_hml" 2>&1; then
        echo -e "${RED}FAIL (original .hml failed)${NC}"
        FAILED=$((FAILED + 1))
        return
    fi

    # Compile to .hmlc
    if ! $HEMLOCK --compile "$test_file" -o "$hmlc_file" --debug > /dev/null 2>&1; then
        echo -e "${RED}FAIL (compilation failed)${NC}"
        FAILED=$((FAILED + 1))
        return
    fi

    # Run compiled .hmlc file
    if ! $HEMLOCK "$hmlc_file" > "$output_hmlc" 2>&1; then
        echo -e "${RED}FAIL (compiled .hmlc failed)${NC}"
        FAILED=$((FAILED + 1))
        return
    fi

    # Compare outputs
    if diff -q "$output_hml" "$output_hmlc" > /dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}FAIL (output mismatch)${NC}"
        echo "  Expected:"
        head -5 "$output_hml" | sed 's/^/    /'
        echo "  Got:"
        head -5 "$output_hmlc" | sed 's/^/    /'
        FAILED=$((FAILED + 1))
    fi
}

# Header
echo "========================================"
echo "  AST Serialization Roundtrip Tests"
echo "========================================"
echo ""

# Run tests in test directory
for test_file in "$TEST_DIR"/*.hml; do
    if [ -f "$test_file" ]; then
        run_test "$test_file"
    fi
done

# Also test some existing test files (if they exist and don't use modules)
EXTRA_TESTS=(
    "tests/primitives/integers.hml"
    "tests/primitives/strings.hml"
    "tests/functions/closures.hml"
    "tests/loops/for_basic.hml"
    "tests/arrays/basic.hml"
    "tests/objects/basic.hml"
)

echo ""
echo "Testing existing test files..."
for test_file in "${EXTRA_TESTS[@]}"; do
    if [ -f "$test_file" ]; then
        # Check if file uses modules (import/export) - skip those
        if grep -qE "^import |^export " "$test_file"; then
            echo "Skipping $test_file (uses modules)"
        else
            run_test "$test_file"
        fi
    fi
done

# Summary
echo ""
echo "========================================"
echo "  Summary: $PASSED passed, $FAILED failed"
echo "========================================"

# Cleanup
rm -rf "$TEMP_DIR"

# Exit with error if any tests failed
if [ $FAILED -gt 0 ]; then
    exit 1
fi
