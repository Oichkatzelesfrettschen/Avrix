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
# Pin to a specific verified commit for security and reproducibility
FLAMEGRAPH_COMMIT="cd9ee4c4449775a2f867acf31c84b7fe4b132ad5"  # v1.0 release

if [ ! -d "${FLAMEGRAPH_DIR}" ]; then
    echo -e "${YELLOW}Cloning FlameGraph tools (pinned to ${FLAMEGRAPH_COMMIT:0:8})...${NC}"
    mkdir -p "${PROJECT_ROOT}/tools"
    if git clone https://github.com/brendangregg/FlameGraph.git "${FLAMEGRAPH_DIR}" 2>&1 | tail -5; then
        cd "${FLAMEGRAPH_DIR}"
        git checkout "${FLAMEGRAPH_COMMIT}" >/dev/null 2>&1
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓ FlameGraph tools installed and verified (commit ${FLAMEGRAPH_COMMIT:0:8})${NC}"
        else
            echo -e "${RED}✗ Failed to checkout pinned commit${NC}"
            cd "${PROJECT_ROOT}"
            rm -rf "${FLAMEGRAPH_DIR}"
            exit 1
        fi
        cd "${PROJECT_ROOT}"
    else
        echo -e "${RED}✗ Failed to clone FlameGraph repository${NC}"
        exit 1
    fi
fi

# Build for profiling
echo -e "${YELLOW}[1/4] Building for profiling...${NC}"
cd "${PROJECT_ROOT}"

# Create log file for full build output
BUILD_LOG="${REPORT_DIR}/build_profiling.log"

if [ ! -d "${BUILD_DIR}" ]; then
    echo "  Configuring build (full output logged to ${BUILD_LOG})..."
    if meson setup "${BUILD_DIR}" \
        -Dbuildtype=debug \
        -Doptimization=2 \
        --wipe > "${BUILD_LOG}" 2>&1; then
        echo -e "${GREEN}  ✓ Configuration complete${NC}"
    else
        echo -e "${RED}  ✗ Configuration failed. Check ${BUILD_LOG} for details${NC}"
        tail -20 "${BUILD_LOG}"
        exit 1
    fi
fi

echo "  Compiling (full output logged to ${BUILD_LOG})..."
if meson compile -C "${BUILD_DIR}" >> "${BUILD_LOG}" 2>&1; then
    echo -e "${GREEN}✓ Build complete${NC}"
else
    echo -e "${RED}✗ Build failed. Last 20 lines of output:${NC}"
    tail -20 "${BUILD_LOG}"
    exit 1
fi
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
    PERF_OUTPUT=$(perf record -o "${REPORT_DIR}/${test_name}.perf.data" -g "$binary" 2>&1)
    PERF_EXIT=$?
    if [ "$PERF_EXIT" -ne 0 ]; then
        if printf '%s\n' "$PERF_OUTPUT" | grep -qiE 'permission denied|are you root|you may not have permission to collect stats'; then
            echo -e "${YELLOW}  ⚠ perf failed due to insufficient privileges (try running with sudo or adjusting perf_event_paranoid)${NC}"
        else
            echo -e "${YELLOW}  ⚠ perf failed with exit code $PERF_EXIT${NC}"
            printf '%s\n' "$PERF_OUTPUT"
        fi
        continue
    fi
    
    # Generate flamegraph only if perf.data was successfully created
    if [ -f "${REPORT_DIR}/${test_name}.perf.data" ] && [ -s "${REPORT_DIR}/${test_name}.perf.data" ]; then
        if perf script -i "${REPORT_DIR}/${test_name}.perf.data" > "${REPORT_DIR}/${test_name}.perf.script" 2>/dev/null; then
            if [ -s "${REPORT_DIR}/${test_name}.perf.script" ]; then
                "${FLAMEGRAPH_DIR}/stackcollapse-perf.pl" "${REPORT_DIR}/${test_name}.perf.script" > "${REPORT_DIR}/${test_name}.folded"
                "${FLAMEGRAPH_DIR}/flamegraph.pl" "${REPORT_DIR}/${test_name}.folded" > "${REPORT_DIR}/${test_name}_flamegraph.svg"
                echo -e "${GREEN}  ✓ Generated flamegraph: ${test_name}_flamegraph.svg${NC}"
            else
                echo -e "${YELLOW}  ⚠ perf script produced no output${NC}"
            fi
        else
            echo -e "${YELLOW}  ⚠ Failed to process perf data${NC}"
        fi
    elif [ -f "${REPORT_DIR}/${test_name}.perf.data" ]; then
        echo -e "${YELLOW}  ⚠ perf.data file is empty, skipping flamegraph generation${NC}"
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
