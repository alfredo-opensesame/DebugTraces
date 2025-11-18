#!/bin/bash
# Run all static analysis and sanitizer checks for DebugTraces library

# Exit on any error
set -e

# Change to project root directory (parent of cmake/)
cd "$(dirname "$0")/.."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "🔍 DebugTraces Quality Assurance Suite"
echo "======================================"
echo

# Track overall success
OVERALL_SUCCESS=true

# Helper function to run a quality check
run_check() {
    local name="$1"
    local preset="$2"
    local icon="$3"
    
    echo -e "${BLUE}${icon} Running ${name}...${NC}"
    
    if cmake --preset "${preset}" > /dev/null 2>&1 && \
       cmake --build "build/${preset}" > /dev/null 2>&1 && \
       cd "build/${preset}" && ctest --output-on-failure > /dev/null 2>&1 && cd ../..; then
        echo -e "${GREEN}✅ ${name} passed${NC}"
        echo
    else
        echo -e "${RED}❌ ${name} failed${NC}"
        echo
        OVERALL_SUCCESS=false
    fi
}

# Run Clang-Tidy Static Analysis (build only, no tests)
echo -e "${BLUE}🔄 Running Clang-Tidy Static Analysis...${NC}"
if cmake --preset clang-tidy > /dev/null 2>&1 && \
   cmake --build build/clang-tidy > /dev/null 2>&1; then
    echo -e "${GREEN}✅ Clang-Tidy Static Analysis passed${NC}"
    echo
else
    echo -e "${RED}❌ Clang-Tidy Static Analysis failed${NC}"
    echo
    OVERALL_SUCCESS=false
fi

# Run sanitizer checks
run_check "AddressSanitizer Tests" "asan" "🔄"
run_check "ThreadSanitizer Tests" "tsan" "🔄"
run_check "UndefinedBehaviorSanitizer Tests" "ubsan" "🔄"

# Run release build compilation check (many tests expected to fail due to disabled logging)
echo -e "${BLUE}🔄 Running Release Build Compilation...${NC}"
if cmake --preset dev-release > /dev/null 2>&1 && \
   cmake --build build/dev-release > /dev/null 2>&1; then
    echo -e "${GREEN}✅ Release Build Compilation passed${NC}"
    echo -e "${YELLOW}ℹ️  Note: Many tests fail in release builds by design (logging disabled)${NC}"
    echo
else
    echo -e "${RED}❌ Release Build Compilation failed${NC}"
    echo
    OVERALL_SUCCESS=false
fi

echo "======================================"
if [ "$OVERALL_SUCCESS" = true ]; then
    echo -e "${GREEN}🎉 All quality checks passed!${NC}"
    echo
    exit 0
else
    echo -e "${RED}💥 Some quality checks failed!${NC}"
    echo
    exit 1
fi