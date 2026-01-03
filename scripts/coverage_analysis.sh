#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════
# Coverage Analysis Script for Avrix
# Uses lcov/gcov to generate code coverage reports
# ═══════════════════════════════════════════════════════════════════════
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build_coverage"
REPORT_DIR="${PROJECT_ROOT}/analysis_reports/coverage"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Avrix Code Coverage Analysis${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo

# Check for required tools
if ! command -v lcov &> /dev/null; then
    echo -e "${RED}✗ lcov not found. Install with: sudo apt-get install lcov${NC}"
    exit 1
fi

if ! command -v genhtml &> /dev/null; then
    echo -e "${RED}✗ genhtml not found (should come with lcov)${NC}"
    exit 1
fi

# Clean previous coverage data
echo -e "${YELLOW}[1/6] Cleaning previous coverage data...${NC}"
rm -rf "${BUILD_DIR}" "${REPORT_DIR}"
mkdir -p "${REPORT_DIR}"

# Configure build with coverage flags
echo -e "${YELLOW}[2/6] Configuring build with coverage instrumentation...${NC}"
cd "${PROJECT_ROOT}"

# Host build with coverage enabled
meson setup "${BUILD_DIR}" \
    -Dcov=true \
    -Db_coverage=true \
    -Dbuildtype=debug \
    -Doptimization=0 \
    --wipe 2>&1 | head -20

echo -e "${GREEN}✓ Configuration complete${NC}"
echo

# Build with coverage
echo -e "${YELLOW}[3/6] Building with coverage instrumentation...${NC}"
meson compile -C "${BUILD_DIR}" 2>&1 | tail -20
echo -e "${GREEN}✓ Build complete${NC}"
echo

# Run tests
echo -e "${YELLOW}[4/6] Running test suite...${NC}"
cd "${BUILD_DIR}"
meson test --print-errorlogs || echo -e "${YELLOW}⚠ Some tests failed${NC}"
echo -e "${GREEN}✓ Tests complete${NC}"
echo

# Generate coverage report
echo -e "${YELLOW}[5/6] Generating coverage report...${NC}"

# Initialize lcov
lcov --capture \
    --directory "${BUILD_DIR}" \
    --output-file "${REPORT_DIR}/coverage.info" \
    --rc lcov_branch_coverage=1 \
    2>&1 | grep -v "ignoring data for external file" | head -20

# Filter out system headers and test files
lcov --remove "${REPORT_DIR}/coverage.info" \
    '/usr/*' \
    '*/tests/*' \
    '*/build*/*' \
    --output-file "${REPORT_DIR}/coverage_filtered.info" \
    --rc lcov_branch_coverage=1 \
    2>&1 | head -10

echo -e "${GREEN}✓ Coverage data collected${NC}"
echo

# Generate HTML report
echo -e "${YELLOW}[6/6] Generating HTML report...${NC}"
genhtml "${REPORT_DIR}/coverage_filtered.info" \
    --output-directory "${REPORT_DIR}/html" \
    --title "Avrix Coverage Report" \
    --legend \
    --rc lcov_branch_coverage=1 \
    2>&1 | tail -10

echo -e "${GREEN}✓ HTML report generated${NC}"
echo

# Display summary
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Coverage Summary${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo

lcov --summary "${REPORT_DIR}/coverage_filtered.info" \
    --rc lcov_branch_coverage=1 2>&1 | grep -A 10 "Summary"

echo
echo -e "${GREEN}HTML Report: ${REPORT_DIR}/html/index.html${NC}"
echo -e "${GREEN}Coverage Data: ${REPORT_DIR}/coverage_filtered.info${NC}"
echo
echo -e "${YELLOW}To view the report:${NC}"
echo "  xdg-open ${REPORT_DIR}/html/index.html"
echo "  or open in a browser: file://${REPORT_DIR}/html/index.html"
echo
