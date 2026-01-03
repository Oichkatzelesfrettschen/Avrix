#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════
# Comprehensive Static Analysis Script for Avrix
# ═══════════════════════════════════════════════════════════════════════
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPORT_DIR="${PROJECT_ROOT}/analysis_reports"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

mkdir -p "${REPORT_DIR}"

echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Avrix Comprehensive Static Analysis Suite${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo

# ═══════════════════════════════════════════════════════════════════════
# 1. CPPCHECK - C/C++ Static Analyzer
# ═══════════════════════════════════════════════════════════════════════
echo -e "${YELLOW}[1/5] Running cppcheck...${NC}"
if command -v cppcheck &> /dev/null; then
    cppcheck \
        --enable=all \
        --inconclusive \
        --std=c11 \
        --platform=unix64 \
        --suppressions-list="${PROJECT_ROOT}/.cppcheck-suppressions" \
        --inline-suppr \
        --xml \
        --xml-version=2 \
        -I"${PROJECT_ROOT}/include" \
        -I"${PROJECT_ROOT}/arch/common" \
        -I"${PROJECT_ROOT}/kernel" \
        "${PROJECT_ROOT}/src" \
        "${PROJECT_ROOT}/kernel" \
        "${PROJECT_ROOT}/drivers" \
        "${PROJECT_ROOT}/arch" \
        "${PROJECT_ROOT}/lib" \
        2> "${REPORT_DIR}/cppcheck_report.xml"
    
    # Generate HTML report if available
    if command -v cppcheck-htmlreport &> /dev/null; then
        cppcheck-htmlreport \
            --file="${REPORT_DIR}/cppcheck_report.xml" \
            --report-dir="${REPORT_DIR}/cppcheck_html" \
            --source-dir="${PROJECT_ROOT}"
        echo -e "${GREEN}✓ Cppcheck HTML report: ${REPORT_DIR}/cppcheck_html/index.html${NC}"
    fi
    
    # Count issues
    CPPCHECK_ISSUES=$(grep -c "<error" "${REPORT_DIR}/cppcheck_report.xml" || echo "0")
    echo -e "${GREEN}✓ Cppcheck complete: ${CPPCHECK_ISSUES} issues found${NC}"
else
    echo -e "${RED}✗ cppcheck not found${NC}"
fi
echo

# ═══════════════════════════════════════════════════════════════════════
# 2. CLANG-TIDY - LLVM-based Linter
# ═══════════════════════════════════════════════════════════════════════
echo -e "${YELLOW}[2/5] Running clang-tidy...${NC}"
if command -v clang-tidy &> /dev/null; then
    # Find all C source files
    find "${PROJECT_ROOT}" \
        -type f \
        \( -name "*.c" -o -name "*.h" \) \
        ! -path "*/build*/*" \
        ! -path "*/.*/*" \
        > "${REPORT_DIR}/source_files.txt"
    
    TOTAL_FILES=$(wc -l < "${REPORT_DIR}/source_files.txt")
    CURRENT=0
    
    echo "Analyzing ${TOTAL_FILES} files..."
    
    # Run clang-tidy on each file
    while IFS= read -r file; do
        ((CURRENT++))
        echo -ne "\r  Progress: ${CURRENT}/${TOTAL_FILES}"
        
        clang-tidy "$file" \
            --quiet \
            --config-file="${PROJECT_ROOT}/.clang-tidy" \
            -- \
            -std=c23 \
            -I"${PROJECT_ROOT}/include" \
            -I"${PROJECT_ROOT}/arch/common" \
            -I"${PROJECT_ROOT}/kernel" \
            -I"${PROJECT_ROOT}/compat" \
            -DAVRIX_BUILD \
            >> "${REPORT_DIR}/clang_tidy_report.txt" 2>&1 || true
    done < "${REPORT_DIR}/source_files.txt"
    
    echo -e "\n${GREEN}✓ Clang-tidy complete${NC}"
    
    # Count warnings
    TIDY_WARNINGS=$(grep -c "warning:" "${REPORT_DIR}/clang_tidy_report.txt" || echo "0")
    echo -e "${GREEN}  Found ${TIDY_WARNINGS} warnings${NC}"
else
    echo -e "${RED}✗ clang-tidy not found${NC}"
fi
echo

# ═══════════════════════════════════════════════════════════════════════
# 3. COMPLEXITY ANALYSIS - Lizard or similar
# ═══════════════════════════════════════════════════════════════════════
echo -e "${YELLOW}[3/5] Running complexity analysis...${NC}"
if command -v lizard &> /dev/null; then
    lizard \
        "${PROJECT_ROOT}/src" \
        "${PROJECT_ROOT}/kernel" \
        "${PROJECT_ROOT}/drivers" \
        -l c \
        --xml > "${REPORT_DIR}/complexity_report.xml" || true
    
    echo -e "${GREEN}✓ Complexity analysis complete${NC}"
elif command -v pmccabe &> /dev/null; then
    find "${PROJECT_ROOT}" \
        -type f -name "*.c" \
        ! -path "*/build*/*" \
        -exec pmccabe {} \; > "${REPORT_DIR}/complexity_report.txt" || true
    
    echo -e "${GREEN}✓ Complexity analysis complete (pmccabe)${NC}"
else
    echo -e "${YELLOW}⚠ No complexity analysis tool found (install lizard or pmccabe)${NC}"
fi
echo

# ═══════════════════════════════════════════════════════════════════════
# 4. SECURITY SCANNING - FlawFinder
# ═══════════════════════════════════════════════════════════════════════
echo -e "${YELLOW}[4/5] Running security scan...${NC}"
if command -v flawfinder &> /dev/null; then
    flawfinder \
        --html \
        --minlevel=0 \
        "${PROJECT_ROOT}/src" \
        "${PROJECT_ROOT}/kernel" \
        "${PROJECT_ROOT}/drivers" \
        > "${REPORT_DIR}/security_report.html"
    
    echo -e "${GREEN}✓ Security scan complete: ${REPORT_DIR}/security_report.html${NC}"
else
    echo -e "${YELLOW}⚠ flawfinder not found (install with: pip3 install flawfinder)${NC}"
fi
echo

# ═══════════════════════════════════════════════════════════════════════
# 5. CODE METRICS - Lines of Code, etc.
# ═══════════════════════════════════════════════════════════════════════
echo -e "${YELLOW}[5/5] Generating code metrics...${NC}"

cat > "${REPORT_DIR}/code_metrics.txt" << 'EOF'
═══════════════════════════════════════════════════════════════════════
AVRIX CODE METRICS REPORT
═══════════════════════════════════════════════════════════════════════

EOF

# Lines of code by directory
echo "Lines of Code by Component:" >> "${REPORT_DIR}/code_metrics.txt"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" >> "${REPORT_DIR}/code_metrics.txt"
for dir in src kernel drivers arch lib examples tests; do
    if [ -d "${PROJECT_ROOT}/${dir}" ]; then
        LOC=$(find "${PROJECT_ROOT}/${dir}" -type f -name "*.c" -exec wc -l {} + 2>/dev/null | tail -1 | awk '{print $1}' || echo "0")
        HEADERS=$(find "${PROJECT_ROOT}/${dir}" -type f -name "*.h" -exec wc -l {} + 2>/dev/null | tail -1 | awk '{print $1}' || echo "0")
        printf "  %-15s %8s lines (C)  %8s lines (H)\n" "${dir}/" "${LOC}" "${HEADERS}" >> "${REPORT_DIR}/code_metrics.txt"
    fi
done

echo "" >> "${REPORT_DIR}/code_metrics.txt"
echo "File Count by Type:" >> "${REPORT_DIR}/code_metrics.txt"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" >> "${REPORT_DIR}/code_metrics.txt"
printf "  C source files:    %5d\n" "$(find "${PROJECT_ROOT}" -name "*.c" ! -path "*/build*/*" | wc -l)" >> "${REPORT_DIR}/code_metrics.txt"
printf "  Header files:      %5d\n" "$(find "${PROJECT_ROOT}" -name "*.h" ! -path "*/build*/*" | wc -l)" >> "${REPORT_DIR}/code_metrics.txt"
printf "  Assembly files:    %5d\n" "$(find "${PROJECT_ROOT}" -name "*.S" -o -name "*.s" ! -path "*/build*/*" | wc -l)" >> "${REPORT_DIR}/code_metrics.txt"
printf "  Meson files:       %5d\n" "$(find "${PROJECT_ROOT}" -name "meson.build" | wc -l)" >> "${REPORT_DIR}/code_metrics.txt"

echo "" >> "${REPORT_DIR}/code_metrics.txt"
echo "Documentation:" >> "${REPORT_DIR}/code_metrics.txt"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" >> "${REPORT_DIR}/code_metrics.txt"
printf "  Markdown files:    %5d\n" "$(find "${PROJECT_ROOT}" -name "*.md" | wc -l)" >> "${REPORT_DIR}/code_metrics.txt"
printf "  Documentation LOC: %5d\n" "$(find "${PROJECT_ROOT}" -name "*.md" -exec wc -l {} + 2>/dev/null | tail -1 | awk '{print $1}' || echo "0")" >> "${REPORT_DIR}/code_metrics.txt"

cat "${REPORT_DIR}/code_metrics.txt"
echo -e "${GREEN}✓ Code metrics generated${NC}"
echo

# ═══════════════════════════════════════════════════════════════════════
# Summary
# ═══════════════════════════════════════════════════════════════════════
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Analysis Complete${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo
echo -e "${GREEN}Reports generated in: ${REPORT_DIR}/${NC}"
ls -lh "${REPORT_DIR}/" | tail -n +2 | awk '{print "  " $9 " (" $5 ")"}'
echo
echo -e "${YELLOW}Next steps:${NC}"
echo "  1. Review reports in ${REPORT_DIR}/"
echo "  2. Fix critical issues identified"
echo "  3. Run 'scripts/coverage_analysis.sh' for code coverage"
echo "  4. Run build system validation"
echo
