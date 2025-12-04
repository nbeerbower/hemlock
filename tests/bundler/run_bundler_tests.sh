#!/bin/bash
# Bundler Test Suite
# Tests the hemlock bundler functionality

HEMLOCK="./hemlock"
TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

PASSED=0
FAILED=0

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

pass() {
    echo -e "${GREEN}PASS${NC}: $1"
    ((PASSED++))
}

fail() {
    echo -e "${RED}FAIL${NC}: $1"
    echo "  $2"
    ((FAILED++))
}

echo "=== Hemlock Bundler Test Suite ==="
echo ""

# Test 1: Bundle single file
echo "Test 1: Bundle single file"
if $HEMLOCK --bundle tests/primitives/binary_literals.hml -o "$TMPDIR/single.hmlc" 2>/dev/null; then
    if [ -f "$TMPDIR/single.hmlc" ]; then
        pass "Single file bundle created"
    else
        fail "Single file bundle" "Output file not created"
    fi
else
    fail "Single file bundle" "Bundle command failed"
fi

# Test 2: Run bundled single file
echo "Test 2: Run bundled single file"
if $HEMLOCK "$TMPDIR/single.hmlc" >/dev/null 2>&1; then
    pass "Single file bundle runs"
else
    fail "Single file bundle runs" "Execution failed"
fi

# Test 3: Bundle with stdlib imports
echo "Test 3: Bundle with stdlib imports"
if $HEMLOCK --bundle tests/stdlib_collections/test_basic.hml -o "$TMPDIR/stdlib.hmlc" 2>/dev/null; then
    if [ -f "$TMPDIR/stdlib.hmlc" ]; then
        pass "Stdlib bundle created"
    else
        fail "Stdlib bundle" "Output file not created"
    fi
else
    fail "Stdlib bundle" "Bundle command failed"
fi

# Test 4: Run stdlib bundle
echo "Test 4: Run bundled stdlib file"
OUTPUT=$($HEMLOCK "$TMPDIR/stdlib.hmlc" 2>&1)
if echo "$OUTPUT" | grep -q "All basic collections tests passed"; then
    pass "Stdlib bundle runs correctly"
else
    fail "Stdlib bundle runs" "Expected output not found"
fi

# Test 5: Bundle multi-module example
echo "Test 5: Bundle multi-module example"
if $HEMLOCK --bundle examples/multi_module/main.hml -o "$TMPDIR/multi.hmlc" 2>/dev/null; then
    pass "Multi-module bundle created"
else
    fail "Multi-module bundle" "Bundle command failed"
fi

# Test 6: Run multi-module bundle
echo "Test 6: Run multi-module bundle"
OUTPUT=$($HEMLOCK "$TMPDIR/multi.hmlc" 2>&1)
if echo "$OUTPUT" | grep -q "Example Complete"; then
    pass "Multi-module bundle runs correctly"
else
    fail "Multi-module bundle runs" "Expected output not found"
fi

# Test 7: Compressed bundle
echo "Test 7: Compressed bundle"
if $HEMLOCK --bundle tests/stdlib_collections/test_basic.hml --compress -o "$TMPDIR/compressed.hmlb" 2>/dev/null; then
    UNCOMPRESSED_SIZE=$(stat -c%s "$TMPDIR/stdlib.hmlc" 2>/dev/null || stat -f%z "$TMPDIR/stdlib.hmlc")
    COMPRESSED_SIZE=$(stat -c%s "$TMPDIR/compressed.hmlb" 2>/dev/null || stat -f%z "$TMPDIR/compressed.hmlb")
    if [ "$COMPRESSED_SIZE" -lt "$UNCOMPRESSED_SIZE" ]; then
        pass "Compressed bundle is smaller ($COMPRESSED_SIZE < $UNCOMPRESSED_SIZE bytes)"
    else
        fail "Compression" "Compressed file not smaller"
    fi
else
    fail "Compressed bundle" "Bundle command failed"
fi

# Test 8: Verbose output
echo "Test 8: Verbose output"
OUTPUT=$($HEMLOCK --bundle examples/multi_module/main.hml --verbose -o "$TMPDIR/verbose.hmlc" 2>&1)
if echo "$OUTPUT" | grep -q "Bundle Summary" && echo "$OUTPUT" | grep -q "Flattened"; then
    pass "Verbose output shows summary"
else
    fail "Verbose output" "Expected verbose information not found"
fi

# Test 9: Output matches original execution
echo "Test 9: Output matches original"
ORIGINAL=$($HEMLOCK examples/multi_module/main.hml 2>&1)
BUNDLED=$($HEMLOCK "$TMPDIR/multi.hmlc" 2>&1)
if [ "$ORIGINAL" = "$BUNDLED" ]; then
    pass "Bundled output matches original"
else
    fail "Output match" "Bundled output differs from original"
fi

# Test 10: Multiple stdlib imports
echo "Test 10: Bundle with multiple stdlib imports"
cat > "$TMPDIR/multi_stdlib.hml" << 'EOF'
import { HashMap } from "@stdlib/collections";
import { now } from "@stdlib/time";

let map = HashMap();
map.set("test", 42);
assert(map.get("test") == 42, "HashMap should work");
print("Multi-stdlib test passed!");
EOF

if $HEMLOCK --bundle "$TMPDIR/multi_stdlib.hml" -o "$TMPDIR/multi_stdlib.hmlc" 2>/dev/null; then
    OUTPUT=$($HEMLOCK "$TMPDIR/multi_stdlib.hmlc" 2>&1)
    if echo "$OUTPUT" | grep -q "Multi-stdlib test passed"; then
        pass "Multiple stdlib imports bundle works"
    else
        fail "Multiple stdlib imports" "Execution failed"
    fi
else
    fail "Multiple stdlib imports bundle" "Bundle command failed"
fi

echo ""
echo "=== Results ==="
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"

if [ $FAILED -gt 0 ]; then
    exit 1
fi
