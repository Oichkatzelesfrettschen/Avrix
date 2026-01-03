#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════
# Flamegraph Generation for Avrix
# Profiles runtime performance and generates flamegraphs
# ═══════════════════════════════════════════════════════════════════════
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build_profile"
REPORT_DIR="${PROJECT_ROOT}/analysis_reports/flamegraph"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Avrix Performance Profiling & Flamegraph Generation${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo

# Check for perf
if ! command -v perf &> /dev/null; then
    echo -e "${RED}✗ perf not found. Install with: sudo apt-get install linux-tools-generic${NC}"
    exit 1
fi

# Create report directory
mkdir -p "${REPORT_DIR}"

# Check if flamegraph tools are available
FLAMEGRAPH_DIR="${PROJECT_ROOT}/tools/FlameGraph"
if [ ! -d "${FLAMEGRAPH_DIR}" ]; then
    echo -e "${YELLOW}Cloning FlameGraph tools...${NC}"
    mkdir -p "${PROJECT_ROOT}/tools"
    git clone https://github.com/brendangregg/FlameGraph.git "${FLAMEGRAPH_DIR}" 2>&1 | tail -5
    echo -e "${GREEN}✓ FlameGraph tools installed${NC}"
fi

# Build for profiling
echo -e "${YELLOW}[1/4] Building for profiling...${NC}"
cd "${PROJECT_ROOT}"

if [ ! -d "${BUILD_DIR}" ]; then
    meson setup "${BUILD_DIR}" \
        -Dbuildtype=debug \
        -Doptimization=2 \
        --wipe 2>&1 | head -20
fi

meson compile -C "${BUILD_DIR}" 2>&1 | tail -10
echo -e "${GREEN}✓ Build complete${NC}"
echo

# Find test executables
echo -e "${YELLOW}[2/4] Finding test executables...${NC}"
TEST_BINARIES=$(find "${BUILD_DIR}" -type f -executable -name "*_test" | head -5)

if [ -z "$TEST_BINARIES" ]; then
    echo -e "${RED}✗ No test binaries found${NC}"
    exit 1
fi

echo "Found test binaries:"
echo "$TEST_BINARIES" | sed 's/^/  /'
echo

# Profile each test binary
echo -e "${YELLOW}[3/4] Profiling test binaries...${NC}"
for binary in $TEST_BINARIES; do
    test_name=$(basename "$binary")
    echo "  Profiling ${test_name}..."
    
    # Check if we can run perf record (requires privileges)
    if ! perf record -o "${REPORT_DIR}/${test_name}.perf.data" -g "$binary" 2>&1; then
        PERF_EXIT=$?
        if [ $PERF_EXIT -eq 1 ]; then
            echo -e "${YELLOW}  ⚠ perf requires root privileges or binary crashed${NC}"
        else
            echo -e "${YELLOW}  ⚠ perf failed with exit code $PERF_EXIT${NC}"
        fi
        continue
    fi
    
    # Generate flamegraph
    if [ -f "${REPORT_DIR}/${test_name}.perf.data" ]; then
        perf script -i "${REPORT_DIR}/${test_name}.perf.data" > "${REPORT_DIR}/${test_name}.perf.script"
        "${FLAMEGRAPH_DIR}/stackcollapse-perf.pl" "${REPORT_DIR}/${test_name}.perf.script" > "${REPORT_DIR}/${test_name}.folded"
        "${FLAMEGRAPH_DIR}/flamegraph.pl" "${REPORT_DIR}/${test_name}.folded" > "${REPORT_DIR}/${test_name}_flamegraph.svg"
        echo -e "${GREEN}  ✓ Generated flamegraph: ${test_name}_flamegraph.svg${NC}"
    fi
done

# Build time profiling (using ninja build system)
echo -e "${YELLOW}[4/4] Profiling build times...${NC}"
cd "${BUILD_DIR}"

# Clean and rebuild with timing
ninja -t clean 2>/dev/null || true
time ninja -j1 > "${REPORT_DIR}/build_log.txt" 2>&1 || true

# Extract build statistics
echo "Build Statistics:" > "${REPORT_DIR}/build_stats.txt"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" >> "${REPORT_DIR}/build_stats.txt"
echo "" >> "${REPORT_DIR}/build_stats.txt"

# Count compilation units
echo "Compilation Units:" >> "${REPORT_DIR}/build_stats.txt"
grep -c "\[.*\].*\.c$" "${REPORT_DIR}/build_log.txt" >> "${REPORT_DIR}/build_stats.txt" || echo "0" >> "${REPORT_DIR}/build_stats.txt"

cat "${REPORT_DIR}/build_stats.txt"
echo -e "${GREEN}✓ Build profiling complete${NC}"
echo

# Summary
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Profiling Complete${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo
echo -e "${GREEN}Reports generated in: ${REPORT_DIR}/${NC}"
ls -lh "${REPORT_DIR}/" 2>/dev/null | tail -n +2 | awk '{print "  " $9 " (" $5 ")"}' || echo "  (No files generated)"
echo
echo -e "${YELLOW}View flamegraphs:${NC}"
find "${REPORT_DIR}" -name "*.svg" 2>/dev/null | sed 's/^/  /' || echo "  No SVG files generated"
echo
