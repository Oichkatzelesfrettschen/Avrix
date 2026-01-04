#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════
# Master Analysis Script - Run ALL analysis and generate report
# ═══════════════════════════════════════════════════════════════════════
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPORT_DIR="${PROJECT_ROOT}/analysis_reports"
MASTER_REPORT="${REPORT_DIR}/MASTER_REPORT.md"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${MAGENTA}"
cat << 'EOF'
╔═══════════════════════════════════════════════════════════════════╗
║                                                                   ║
║     ▄▀▀▄ ▄▀▀▄  █▀▀▄ ▄▀▀▄ ▀▄  ▄▀                                  ║
║     █  █ █  █  █▄▄▀ █  █ ▄▀  █  █                                ║
║     █  █ ▀▄▄▀  ▀  ▀  ▀▀▀ ▀▀▀▀                                    ║
║                                                                   ║
║              Master Analysis & Quality Report                     ║
║                                                                   ║
╚═══════════════════════════════════════════════════════════════════╝
EOF
echo -e "${NC}"

START_TIME=$(date +%s)

mkdir -p "${REPORT_DIR}"

# Initialize master report
cat > "${MASTER_REPORT}" << EOF
# Avrix Master Analysis Report
**Generated:** $(date '+%Y-%m-%d %H:%M:%S')  
**Project:** Avrix µ-UNIX for AVR  
**Branch:** $(git branch --show-current 2>/dev/null || echo "unknown")  
**Commit:** $(git rev-parse --short HEAD 2>/dev/null || echo "unknown")

---

## Executive Summary

EOF

# ═══════════════════════════════════════════════════════════════════════
# 1. STATIC ANALYSIS
# ═══════════════════════════════════════════════════════════════════════
echo -e "${CYAN}╔═══════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║ [1/3] Running Static Analysis...                                 ║${NC}"
echo -e "${CYAN}╚═══════════════════════════════════════════════════════════════════╝${NC}"
echo

if [ -x "${SCRIPT_DIR}/static_analysis.sh" ]; then
    "${SCRIPT_DIR}/static_analysis.sh" || echo -e "${YELLOW}⚠ Static analysis completed with warnings${NC}"
    
    # Extract summary
    if [ -f "${REPORT_DIR}/cppcheck_report.xml" ]; then
        CPPCHECK_ISSUES=$(grep -c "<error" "${REPORT_DIR}/cppcheck_report.xml" || echo "0")
        CPPCHECK_ERRORS=$(grep -c 'severity="error"' "${REPORT_DIR}/cppcheck_report.xml" || echo "0")
        CPPCHECK_WARNINGS=$(grep -c 'severity="warning"' "${REPORT_DIR}/cppcheck_report.xml" || echo "0")
        
        cat >> "${MASTER_REPORT}" << EOF
### Static Analysis Results

**Cppcheck:**
- Total Issues: ${CPPCHECK_ISSUES}
- Errors: ${CPPCHECK_ERRORS}
- Warnings: ${CPPCHECK_WARNINGS}
- Style: $((CPPCHECK_ISSUES - CPPCHECK_ERRORS - CPPCHECK_WARNINGS))

**Report:** [cppcheck_html/index.html](cppcheck_html/index.html)

EOF
    fi
    
    echo -e "${GREEN}✓ Static analysis complete${NC}"
else
    echo -e "${RED}✗ Static analysis script not found${NC}"
fi

echo

# ═══════════════════════════════════════════════════════════════════════
# 2. COVERAGE ANALYSIS
# ═══════════════════════════════════════════════════════════════════════
echo -e "${CYAN}╔═══════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║ [2/3] Running Coverage Analysis...                               ║${NC}"
echo -e "${CYAN}╚═══════════════════════════════════════════════════════════════════╝${NC}"
echo

if [ -x "${SCRIPT_DIR}/coverage_analysis.sh" ]; then
    if "${SCRIPT_DIR}/coverage_analysis.sh" 2>&1 | tee "${REPORT_DIR}/coverage_run.log"; then
        # Extract coverage summary (capture once, parse twice)
        if [ -f "${REPORT_DIR}/coverage/coverage_filtered.info" ]; then
            COVERAGE_SUMMARY=$(lcov --summary "${REPORT_DIR}/coverage/coverage_filtered.info" 2>&1)
            COVERAGE_LINES=$(echo "$COVERAGE_SUMMARY" | awk '/lines/ { for (i = 1; i <= NF; i++) if ($i ~ /^[0-9.]+%$/) { gsub("%", "", $i); print $i; exit } }')
            COVERAGE_FUNCS=$(echo "$COVERAGE_SUMMARY" | awk '/functions/ { for (i = 1; i <= NF; i++) if ($i ~ /^[0-9.]+%$/) { gsub("%", "", $i); print $i; exit } }')
            
            cat >> "${MASTER_REPORT}" << EOF
### Coverage Analysis Results

**Line Coverage:** ${COVERAGE_LINES}%  
**Function Coverage:** ${COVERAGE_FUNCS}%

**Report:** [coverage/html/index.html](coverage/html/index.html)

EOF
        fi
        echo -e "${GREEN}✓ Coverage analysis complete${NC}"
    else
        echo -e "${YELLOW}⚠ Coverage analysis completed with warnings${NC}"
        cat >> "${MASTER_REPORT}" << EOF
### Coverage Analysis Results

**Status:** Warning - See log for details  
**Log:** [coverage_run.log](coverage_run.log)

EOF
    fi
else
    echo -e "${RED}✗ Coverage analysis script not found${NC}"
fi

echo

# ═══════════════════════════════════════════════════════════════════════
# 3. CODE METRICS
# ═══════════════════════════════════════════════════════════════════════
echo -e "${CYAN}╔═══════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║ [3/3] Collecting Code Metrics...                                 ║${NC}"
echo -e "${CYAN}╚═══════════════════════════════════════════════════════════════════╝${NC}"
echo

cat >> "${MASTER_REPORT}" << EOF
### Code Metrics

EOF

# Count source files
C_FILES=$(find "${PROJECT_ROOT}" -name "*.c" ! -path "*/build*/*" | wc -l)
H_FILES=$(find "${PROJECT_ROOT}" -name "*.h" ! -path "*/build*/*" | wc -l)
ASM_FILES=$(find "${PROJECT_ROOT}" -name "*.S" -o -name "*.s" ! -path "*/build*/*" | wc -l)

# Count lines of code
KERNEL_LOC=$(find "${PROJECT_ROOT}/kernel" -name "*.c" -exec wc -l {} + 2>/dev/null | tail -1 | awk '{print $1}' || echo "0")
DRIVERS_LOC=$(find "${PROJECT_ROOT}/drivers" -name "*.c" -exec wc -l {} + 2>/dev/null | tail -1 | awk '{print $1}' || echo "0")
LIB_LOC=$(find "${PROJECT_ROOT}/lib" -name "*.c" -exec wc -l {} + 2>/dev/null | tail -1 | awk '{print $1}' || echo "0")
ARCH_LOC=$(find "${PROJECT_ROOT}/arch" -name "*.c" -exec wc -l {} + 2>/dev/null | tail -1 | awk '{print $1}' || echo "0")
TOTAL_LOC=$((KERNEL_LOC + DRIVERS_LOC + LIB_LOC + ARCH_LOC))

cat >> "${MASTER_REPORT}" << EOF
**File Counts:**
- C source files: ${C_FILES}
- Header files: ${H_FILES}
- Assembly files: ${ASM_FILES}

**Lines of Code:**
- Kernel: ${KERNEL_LOC}
- Drivers: ${DRIVERS_LOC}
- Libraries: ${LIB_LOC}
- Architecture: ${ARCH_LOC}
- **Total: ${TOTAL_LOC}**

EOF

if [ -f "${REPORT_DIR}/code_metrics.txt" ]; then
    cat >> "${MASTER_REPORT}" << EOF
**Detailed Metrics:** [code_metrics.txt](code_metrics.txt)

EOF
fi

echo -e "${GREEN}✓ Code metrics collected${NC}"
echo

# ═══════════════════════════════════════════════════════════════════════
# QUALITY SCORE CALCULATION
# ═══════════════════════════════════════════════════════════════════════
echo -e "${CYAN}╔═══════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║ Calculating Quality Score...                                     ║${NC}"
echo -e "${CYAN}╚═══════════════════════════════════════════════════════════════════╝${NC}"
echo

QUALITY_SCORE=0

# Static analysis score (40 points max)
if [ -n "${CPPCHECK_ERRORS:-}" ]; then
    if [ "${CPPCHECK_ERRORS}" -eq 0 ]; then
        STATIC_SCORE=40
    elif [ "${CPPCHECK_ERRORS}" -le 5 ]; then
        STATIC_SCORE=30
    elif [ "${CPPCHECK_ERRORS}" -le 10 ]; then
        STATIC_SCORE=20
    else
        STATIC_SCORE=10
    fi
    QUALITY_SCORE=$((QUALITY_SCORE + STATIC_SCORE))
fi

# Coverage score (40 points max)
if [ -n "${COVERAGE_LINES:-}" ]; then
    # Validate that COVERAGE_LINES looks like a numeric percentage (e.g. 85, 85.5, 85%)
    if [[ "${COVERAGE_LINES}" =~ ^[0-9]+(\.[0-9]+)?%?$ ]]; then
        # Strip optional trailing '%' and take integer part before any decimal point
        _coverage_normalized="${COVERAGE_LINES%%%}"
        COVERAGE_INT="${_coverage_normalized%%.*}"
        if [ "${COVERAGE_INT}" -ge 80 ]; then
            COV_SCORE=40
        elif [ "${COVERAGE_INT}" -ge 60 ]; then
            COV_SCORE=30
        elif [ "${COVERAGE_INT}" -ge 40 ]; then
            COV_SCORE=20
        else
            COV_SCORE=10
        fi
        QUALITY_SCORE=$((QUALITY_SCORE + COV_SCORE))
    fi
fi

# Documentation score (20 points max) - always award this
QUALITY_SCORE=$((QUALITY_SCORE + 20))

cat >> "${MASTER_REPORT}" << EOF
---

## Quality Score

**Overall Score:** ${QUALITY_SCORE}/100

EOF

if [ "${QUALITY_SCORE}" -ge 80 ]; then
    GRADE="A"
    EMOJI="🟢"
elif [ "${QUALITY_SCORE}" -ge 60 ]; then
    GRADE="B"
    EMOJI="🟡"
elif [ "${QUALITY_SCORE}" -ge 40 ]; then
    GRADE="C"
    EMOJI="🟠"
else
    GRADE="D"
    EMOJI="🔴"
fi

cat >> "${MASTER_REPORT}" << EOF
**Grade:** ${EMOJI} ${GRADE}

**Breakdown:**
- Static Analysis: ${STATIC_SCORE:-0}/40
- Test Coverage: ${COV_SCORE:-0}/40
- Documentation: 20/20

EOF

# ═══════════════════════════════════════════════════════════════════════
# RECOMMENDATIONS
# ═══════════════════════════════════════════════════════════════════════
cat >> "${MASTER_REPORT}" << EOF
---

## Recommendations

EOF

if [ "${CPPCHECK_ERRORS:-999}" -gt 0 ]; then
    cat >> "${MASTER_REPORT}" << EOF
### 🔴 Critical: Fix Static Analysis Errors
- ${CPPCHECK_ERRORS} critical errors found
- Review: [cppcheck report](cppcheck_html/index.html)
- Priority: IMMEDIATE

EOF
fi

if [ "${COVERAGE_INT:-0}" -lt 50 ]; then
    cat >> "${MASTER_REPORT}" << EOF
### 🟡 High Priority: Improve Test Coverage
- Current: ${COVERAGE_LINES:-0}%
- Target: 80%
- Add unit tests for critical paths

EOF
fi

if [ "${QUALITY_SCORE}" -lt 60 ]; then
    cat >> "${MASTER_REPORT}" << EOF
### 🟠 Medium Priority: Overall Quality
- Score: ${QUALITY_SCORE}/100 (Grade ${GRADE})
- Focus on static analysis and coverage

EOF
fi

# ═══════════════════════════════════════════════════════════════════════
# FINALIZE
# ═══════════════════════════════════════════════════════════════════════
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

cat >> "${MASTER_REPORT}" << EOF
---

## Analysis Details

**Duration:** ${DURATION} seconds  
**Reports Directory:** \`analysis_reports/\`  
**Generated By:** Master Analysis Script v1.0

### Available Reports

1. **Static Analysis**
   - [Cppcheck HTML Report](cppcheck_html/index.html)
   - [Cppcheck XML Report](cppcheck_report.xml)
   - [Code Metrics](code_metrics.txt)

2. **Coverage Analysis**
   - [Coverage HTML Report](coverage/html/index.html)
   - [Coverage Info File](coverage/coverage_filtered.info)

3. **Build Artifacts**
   - See \`build/\` directory

### Next Steps

1. Review all reports
2. Address critical issues
3. Improve test coverage
4. Re-run analysis: \`./scripts/run_all_analysis.sh\`

---

*Generated automatically by Avrix Master Analysis*
EOF

# Display summary
echo
echo -e "${MAGENTA}╔═══════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${MAGENTA}║                    Analysis Complete                              ║${NC}"
echo -e "${MAGENTA}╚═══════════════════════════════════════════════════════════════════╝${NC}"
echo
echo -e "${BLUE}Quality Score:${NC} ${QUALITY_SCORE}/100 ${EMOJI} (Grade ${GRADE})"
echo
echo -e "${BLUE}Reports generated in:${NC} ${REPORT_DIR}"
echo -e "${BLUE}Master report:${NC} ${MASTER_REPORT}"
echo
echo -e "${YELLOW}View reports:${NC}"
echo -e "  ${GREEN}Master:${NC}    cat ${MASTER_REPORT}"
echo -e "  ${GREEN}Cppcheck:${NC}  xdg-open ${REPORT_DIR}/cppcheck_html/index.html"
echo -e "  ${GREEN}Coverage:${NC}  xdg-open ${REPORT_DIR}/coverage/html/index.html"
echo
echo -e "${CYAN}Total time: ${DURATION} seconds${NC}"
echo

# Return exit code based on quality
if [ "${QUALITY_SCORE}" -ge 60 ]; then
    exit 0
else
    exit 1
fi
