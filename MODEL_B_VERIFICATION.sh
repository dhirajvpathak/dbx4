#!/bin/bash

echo "════════════════════════════════════════════════════"
echo "MODEL B COMPREHENSIVE VERIFICATION"
echo "════════════════════════════════════════════════════"

echo ""
echo "STEP 1: Build Quality Check"
echo "────────────────────────────"
echo "Source files in src/:"
find src -name "*.cpp" | wc -l
echo ""
echo "CMakeLists references:"
grep "src/" CMakeLists.txt | grep ".cpp" | wc -l
echo ""

echo "STEP 2: Compilation Check"
echo "────────────────────────────"
cd build 2>/dev/null || (mkdir -p build && cd build && cmake -S .. -B . > /dev/null 2>&1)
cmake --build . -j24 2>&1 | grep -E "error:|Built target" | tail -10
echo ""

echo "STEP 3: Test Execution"
echo "────────────────────────────"
ctest --timeout 120 2>&1 | tail -5
echo ""

echo "STEP 4: Recovery Test Analysis"
echo "────────────────────────────────"
cd ..
echo "Recovery tests found:"
ls -1 tests/test_*recovery*.cpp
echo ""
echo "Check 1: Uses real dbx4::Database header?"
grep -l "#include.*dbx4/database.h" tests/test_*recovery*.cpp 2>/dev/null || echo "  ❌ None use production header"
echo ""
echo "Check 2: Manual WAL file creation?"
grep -l "std::ofstream\|std::ifstream" tests/test_*recovery*.cpp 2>/dev/null && echo "  ❌ Manual WAL simulation detected" || echo "  ✅ No manual WAL"
echo ""
echo "Check 3: Defines local Database class?"
grep -l "class Database" tests/test_*recovery*.cpp 2>/dev/null && echo "  ❌ Local Database class found" || echo "  ✅ No local class"
echo ""

echo "STEP 5: ODR Violation Check"
echo "────────────────────────────"
echo "database.cpp size:"
wc -l src/database.cpp | awk '{print $1 " lines"}'
echo ""
echo "query_executor_public_api.cpp size:"
wc -l src/query_executor_public_api.cpp | awk '{print $1 " lines"}'
echo ""

echo "STEP 6: Key Production Files"
echo "────────────────────────────"
for file in recovery_engine_production.cpp sql_parser.cpp query_executor_engine.cpp transaction_manager.cpp wal_codec_robust.cpp; do
  if [ -f "src/$file" ]; then
    lines=$(wc -l < "src/$file")
    echo "✓ $file: $lines lines"
  else
    echo "❌ $file: NOT FOUND"
  fi
done
echo ""

echo "STEP 7: Final Summary"
echo "────────────────────────"
echo "Build Status: $(cd build && cmake --build . 2>&1 | grep -q 'Built target' && echo '✅ PASS' || echo '❌ FAIL')"
echo "Test Status: $(cd build && ctest 2>&1 | grep -q '44 tests passed' && echo '✅ 44/44 PASS' || echo '❌ FAIL')"
echo ""

