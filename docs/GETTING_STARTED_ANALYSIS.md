# Getting Started with Avrix Analysis Tools

This guide helps you quickly set up and use the comprehensive analysis infrastructure for Avrix.

## 🚀 Quick Start (5 minutes)

### 1. Install Dependencies

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    cppcheck \
    clang-tidy \
    lcov \
    valgrind \
    python3-pip

# Optional: Install flawfinder for security scanning
pip3 install flawfinder lizard

# macOS (using Homebrew)
brew install cppcheck llvm lcov

# Arch Linux
sudo pacman -S cppcheck clang lcov valgrind
```

### 2. Run Analysis

```bash
# Navigate to project root
cd /path/to/Avrix

# Option A: Run all analysis at once (recommended)
./scripts/run_all_analysis.sh

# Option B: Run individually
./scripts/static_analysis.sh      # ~2 minutes
./scripts/coverage_analysis.sh    # ~5 minutes
./scripts/flamegraph_analysis.sh  # ~3 minutes (requires sudo)
```

### 3. View Results

```bash
# Open HTML reports in browser
xdg-open analysis_reports/cppcheck_html/index.html
xdg-open analysis_reports/coverage/html/index.html

# Or view text reports
cat analysis_reports/code_metrics.txt
cat analysis_reports/MASTER_REPORT.md
```

---

## 📊 What You'll Get

### Static Analysis Report
- **Cppcheck:** 183 issues identified (2 critical)
- **Clang-tidy:** Style and correctness checks
- **Complexity:** Cyclomatic complexity metrics
- **Security:** Vulnerability scanning

**Output:** `analysis_reports/cppcheck_html/index.html`

### Coverage Report
- **Line Coverage:** X% (target: 80%)
- **Function Coverage:** Y% (target: 90%)
- **Branch Coverage:** Z% (target: 70%)

**Output:** `analysis_reports/coverage/html/index.html`

### Performance Report
- **Flamegraphs:** Visual performance profiling
- **Build Times:** Compilation bottlenecks
- **Hot Spots:** CPU-intensive functions

**Output:** `analysis_reports/flamegraph/*.svg`

---

## 🎯 Interpreting Results

### Quality Score

The master script calculates an overall quality score (0-100):

```
Score = Static Analysis (40) + Coverage (40) + Documentation (20)

Grades:
  80-100: A 🟢 Excellent
  60-79:  B 🟡 Good
  40-59:  C 🟠 Fair
  0-39:   D 🔴 Poor
```

### Priority Levels

Reports use color-coding:

- 🔴 **Critical** - Fix immediately (build-breaking)
- 🟡 **High** - Fix before merge (potential bugs)
- 🟠 **Medium** - Fix for quality (technical debt)
- 🟢 **Low** - Optional (style/info)

---

## 🛠️ Common Workflows

### Before Committing Code

```bash
# Quick checks (fast)
./scripts/static_analysis.sh
# Fix any errors
# Commit

# Full validation (slower)
./scripts/run_all_analysis.sh
# Review score
# Ensure Grade >= B
```

### Weekly Quality Report

```bash
# Run full analysis
./scripts/run_all_analysis.sh

# Review master report
cat analysis_reports/MASTER_REPORT.md

# Track metrics over time
echo "$(date),$(grep 'Quality Score' analysis_reports/MASTER_REPORT.md)" >> quality_history.csv
```

### Debugging Performance Issues

```bash
# Profile specific test
./scripts/flamegraph_analysis.sh

# Identify hot spots in flamegraph
firefox analysis_reports/flamegraph/*_flamegraph.svg

# Optimize identified functions
vim src/hot_function.c
```

### Improving Coverage

```bash
# Identify uncovered code
./scripts/coverage_analysis.sh
firefox analysis_reports/coverage/html/index.html

# Add tests for red/yellow sections
vim tests/new_test.c

# Re-run to verify
./scripts/coverage_analysis.sh
```

---

## 📚 Documentation

Full documentation available:

1. **[scripts/README.md](README.md)** - Script details
2. **[docs/TECHNICAL_DEBT_ANALYSIS.md](../docs/TECHNICAL_DEBT_ANALYSIS.md)** - Debt quantification
3. **[docs/BUILD_SYSTEM_MODERNIZATION.md](../docs/BUILD_SYSTEM_MODERNIZATION.md)** - Build optimization
4. **[docs/UNIX_DESIGN_PATTERNS_RESEARCH.md](../docs/UNIX_DESIGN_PATTERNS_RESEARCH.md)** - Design insights
5. **[docs/COMPREHENSIVE_RD_REPORT.md](../docs/COMPREHENSIVE_RD_REPORT.md)** - Executive summary

---

## 🐛 Troubleshooting

### "cppcheck: command not found"
```bash
sudo apt install cppcheck
# OR
brew install cppcheck
```

### "Permission denied" for perf
```bash
# Option 1: Run with sudo
sudo ./scripts/flamegraph_analysis.sh

# Option 2: Lower security (temporary)
echo 0 | sudo tee /proc/sys/kernel/perf_event_paranoid

# Option 3: Skip flamegraph analysis
./scripts/static_analysis.sh
./scripts/coverage_analysis.sh
```

### Coverage shows 0%
```bash
# Ensure debug build
rm -rf build_coverage
meson setup build_coverage -Db_coverage=true -Dbuildtype=debug
./scripts/coverage_analysis.sh
```

### Out of disk space
```bash
# Clean old reports
rm -rf analysis_reports/

# Clean build directories
rm -rf build*/

# Re-run analysis
./scripts/run_all_analysis.sh
```

---

## 🔄 CI/CD Integration

### GitHub Actions

Add to `.github/workflows/quality.yml`:

```yaml
name: Code Quality

on: [push, pull_request]

jobs:
  analysis:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cppcheck lcov
      
      - name: Run analysis
        run: ./scripts/run_all_analysis.sh
      
      - name: Upload reports
        uses: actions/upload-artifact@v4
        with:
          name: analysis-reports
          path: analysis_reports/
      
      - name: Fail on poor quality
        run: |
          SCORE=$(grep "Overall Score" analysis_reports/MASTER_REPORT.md | grep -oP '\d+')
          if [ "$SCORE" -lt 60 ]; then
            echo "Quality score $SCORE is below threshold (60)"
            exit 1
          fi
```

### Pre-commit Hook

Add to `.git/hooks/pre-commit`:

```bash
#!/bin/bash
./scripts/static_analysis.sh || exit 1
echo "Static analysis passed!"
```

---

## 📈 Tracking Progress

### Create Quality Dashboard

```bash
# Track metrics over time
cat > track_quality.sh << 'EOF'
#!/bin/bash
./scripts/run_all_analysis.sh
DATE=$(date '+%Y-%m-%d')
SCORE=$(grep "Overall Score" analysis_reports/MASTER_REPORT.md | grep -oP '\d+')
echo "$DATE,$SCORE" >> quality_history.csv
EOF

chmod +x track_quality.sh

# Run weekly
crontab -e
# Add: 0 9 * * 1 cd /path/to/Avrix && ./track_quality.sh
```

### Generate Charts

```python
# plot_quality.py
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('quality_history.csv', names=['date', 'score'])
df['date'] = pd.to_datetime(df['date'])

plt.figure(figsize=(10, 6))
plt.plot(df['date'], df['score'], marker='o')
plt.axhline(y=80, color='g', linestyle='--', label='Target (A)')
plt.axhline(y=60, color='y', linestyle='--', label='Threshold (B)')
plt.xlabel('Date')
plt.ylabel('Quality Score')
plt.title('Avrix Quality Score Over Time')
plt.legend()
plt.grid(True)
plt.savefig('quality_trend.png')
print("Chart saved to quality_trend.png")
```

---

## 🎓 Learning Resources

- **Static Analysis:** [Cppcheck Manual](http://cppcheck.net/manual.pdf)
- **Coverage:** [lcov Documentation](http://ltp.sourceforge.net/coverage/lcov.php)
- **Profiling:** [FlameGraph Guide](http://www.brendangregg.com/flamegraphs.html)
- **Best Practices:** [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)

---

## ❓ Getting Help

1. **Check documentation:** `cat scripts/README.md`
2. **Review examples:** See CI/CD workflows in `.github/workflows/`
3. **Open issue:** Include output from `./scripts/run_all_analysis.sh`

---

## 🎉 Success!

You're now ready to:

- ✅ Run comprehensive code analysis
- ✅ Measure test coverage
- ✅ Profile performance
- ✅ Track quality over time
- ✅ Integrate with CI/CD

**Next steps:**
1. Run `./scripts/run_all_analysis.sh`
2. Review `analysis_reports/MASTER_REPORT.md`
3. Fix critical issues
4. Aim for Grade A (80+)!

---

**Questions?** See full documentation in `docs/` directory.

**Last Updated:** 2026-01-03
