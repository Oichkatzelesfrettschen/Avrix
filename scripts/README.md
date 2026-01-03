# Avrix Analysis & Build Scripts

This directory contains comprehensive scripts for analyzing, profiling, and building Avrix.

## Quick Start

```bash
# Run all analysis
./scripts/static_analysis.sh      # Static analysis (cppcheck, clang-tidy)
./scripts/coverage_analysis.sh    # Code coverage (lcov/gcov)
./scripts/flamegraph_analysis.sh  # Performance profiling

# View results
xdg-open ../analysis_reports/cppcheck_html/index.html
xdg-open ../analysis_reports/coverage/html/index.html
```

## Scripts Overview

### 1. static_analysis.sh
**Purpose:** Comprehensive static code analysis

**Tools Used:**
- **cppcheck:** C/C++ static analyzer
- **clang-tidy:** LLVM-based linter
- **lizard/pmccabe:** Cyclomatic complexity
- **flawfinder:** Security scanning

**Output:**
```
analysis_reports/
├── cppcheck_report.xml
├── cppcheck_html/index.html
├── clang_tidy_report.txt
├── complexity_report.xml
├── security_report.html
└── code_metrics.txt
```

**Usage:**
```bash
./scripts/static_analysis.sh

# Fix issues found
vim src/file.c  # Address issues
./scripts/static_analysis.sh  # Re-run
```

**Time:** ~2-3 minutes

---

### 2. coverage_analysis.sh
**Purpose:** Measure test coverage

**Tools Used:**
- **lcov:** Coverage info collection
- **genhtml:** HTML report generation
- **gcov:** GCC coverage tool

**Prerequisites:**
```bash
sudo apt install lcov
```

**Output:**
```
analysis_reports/coverage/
├── coverage.info
├── coverage_filtered.info
└── html/index.html
```

**Usage:**
```bash
./scripts/coverage_analysis.sh

# View in browser
firefox analysis_reports/coverage/html/index.html
```

**Time:** ~5-7 minutes (includes full test run)

---

### 3. flamegraph_analysis.sh
**Purpose:** Profile performance and generate flamegraphs

**Tools Used:**
- **perf:** Linux performance profiler
- **FlameGraph:** Visualization toolkit (pinned to commit cd9ee4c4 for security)

**Security Features:**
- Pinned to verified v1.0 release commit (cd9ee4c4)
- Commit hash verification after clone
- Perl syntax validation of scripts
- Required file existence checks
- Automatic verification on each run

**Prerequisites:**
```bash
sudo apt install linux-tools-generic
# OR: sudo apt install linux-tools-$(uname -r)

# For manual/offline installation (optional):
# If you prefer to vendor FlameGraph instead of auto-download:
cd tools/
git clone https://github.com/brendangregg/FlameGraph.git
cd FlameGraph && git checkout cd9ee4c4449775a2f867acf31c84b7fe4b132ad5
```

**Output:**
```
analysis_reports/flamegraph/
├── test_name_flamegraph.svg
├── test_name.perf.data
└── build_stats.txt
```

**Usage:**
```bash
./scripts/flamegraph_analysis.sh

# View flamegraph
firefox analysis_reports/flamegraph/*_flamegraph.svg
```

**Note:** May require `sudo` for perf profiling

**Time:** ~3-5 minutes

---

## Integration with CI/CD

### GitHub Actions

Add to `.github/workflows/ci.yml`:

```yaml
- name: Static Analysis
  run: ./scripts/static_analysis.sh

- name: Coverage Analysis
  run: ./scripts/coverage_analysis.sh

- name: Upload Coverage
  uses: codecov/codecov-action@v3
  with:
    files: ./analysis_reports/coverage/coverage_filtered.info
```

### GitLab CI

Add to `.gitlab-ci.yml`:

```yaml
static-analysis:
  script:
    - ./scripts/static_analysis.sh
  artifacts:
    paths:
      - analysis_reports/

coverage:
  script:
    - ./scripts/coverage_analysis.sh
  coverage: '/lines......: (\d+\.\d+)%/'
  artifacts:
    paths:
      - analysis_reports/coverage/
```

---

## Build Helper Scripts

### flash.sh
**Purpose:** Flash firmware to AVR target

**Usage:**
```bash
./scripts/flash.sh build/unix0.hex
./scripts/flash.sh -p /dev/ttyUSB0 -c arduino build/unix0.hex
```

### verify_profiles.sh
**Purpose:** Verify PSE51/PSE52/PSE54 profiles build correctly

**Usage:**
```bash
./scripts/verify_profiles.sh
```

---

## Advanced Usage

### Running Analysis on Specific Directories

```bash
# Modify scripts to target specific paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${PROJECT_ROOT:-$(cd "${SCRIPT_DIR}/.." && pwd)}"

# Analyze only kernel
cppcheck "${PROJECT_ROOT}/kernel" -I"${PROJECT_ROOT}/include" ...
```

### Parallel Analysis

```bash
# Run multiple analyses in parallel
./scripts/static_analysis.sh &
./scripts/coverage_analysis.sh &
wait
echo "All analysis complete!"
```

### Automated Weekly Reports

```bash
# Add to crontab
0 2 * * 1 cd /path/to/Avrix && ./scripts/static_analysis.sh > weekly_report.txt 2>&1
```

---

## Interpreting Results

### Static Analysis

**Priority Levels:**
- **Error:** Fix immediately (may break build)
- **Warning:** Fix before merging (potential bugs)
- **Style:** Fix for consistency (optional)
- **Information:** Review (may be false positives)

**Common Issues:**
```
missingInclude     → Add #include directive
constVariable      → Add const qualifier
unusedFunction     → Remove or mark static
nullPointer        → Add NULL check
```

### Coverage

**Target Thresholds:**
- **Lines:** > 80%
- **Functions:** > 90%
- **Branches:** > 70%

**Colors in HTML:**
- 🟢 Green (≥90%): Well tested
- 🟡 Yellow (50-90%): Needs improvement
- 🔴 Red (<50%): Critical gap

### Flamegraphs

**Reading Flamegraphs:**
- **Width:** % of time spent in function
- **Height:** Call stack depth
- **Color:** Random (for contrast)

**Hot Spots:**
- Wide boxes at top → optimization targets
- Many thin boxes → fragmented execution
- Deep stacks → recursion or deep calls

---

## Troubleshooting

### "cppcheck not found"
```bash
sudo apt install cppcheck
```

### "clang-tidy not found"
```bash
sudo apt install clang-tidy
```

### "lcov not found"
```bash
sudo apt install lcov
```

### "perf not found"
```bash
sudo apt install linux-tools-generic
# OR
sudo apt install linux-tools-$(uname -r)
```

### "Permission denied" for perf
```bash
# Option 1: Run with sudo
sudo ./scripts/flamegraph_analysis.sh

# Option 2: Adjust perf_event_paranoid
echo 0 | sudo tee /proc/sys/kernel/perf_event_paranoid
```

### Coverage shows 0%
```bash
# Ensure build has coverage flags
meson configure build_coverage -Db_coverage=true
meson compile -C build_coverage
```

---

## Contributing

When adding new scripts:

1. **Follow naming convention:** `action_target.sh`
2. **Add help text:** `./script.sh --help`
3. **Add to this README**
4. **Make executable:** `chmod +x script.sh`
5. **Use bash strict mode:** `set -euo pipefail`
6. **Add progress indicators**
7. **Generate reports in `analysis_reports/`**

---

## References

- [Cppcheck Manual](http://cppcheck.net/manual.pdf)
- [Clang-Tidy Checks](https://clang.llvm.org/extra/clang-tidy/checks/list.html)
- [lcov Documentation](http://ltp.sourceforge.net/coverage/lcov.php)
- [FlameGraph](https://github.com/brendangregg/FlameGraph)
- [Linux perf](https://perf.wiki.kernel.org/index.php/Main_Page)

---

**Last Updated:** 2026-01-03  
**Maintainer:** Avrix Development Team
