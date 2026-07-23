# DBX4 LOCAL DEVELOPMENT ENVIRONMENT SETUP
## Windows 10/11 with Visual Studio 2022

---

## PREREQUISITES CHECKLIST

### 1. Visual Studio 2022 Installation
- [ ] Visual Studio 2022 Professional/Community Edition
- [ ] C++ Workload installed
- [ ] CMake Tools for Windows
- [ ] Git Tools

**Verify Installation:**
```cmd
# Check MSVC compiler
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\bin\Hostx64\x64\cl.exe" /?

# Check CMake
cmake --version

# Check Git
git --version
```

### 2. Required Tools
- [ ] CMake 3.20+
- [ ] Git (for cloning repo)
- [ ] 7-Zip or WinRAR (for file extraction)

---

## STEP 1: CLONE THE REPOSITORY

### 1.1 Create Working Directory
```cmd
# Create folder
mkdir C:\Dhiraj\DBX4
cd C:\Dhiraj\DBX4

# Clone repository
git clone https://github.com/dhirajvpathak/dbx4.git
cd dbx4
```

### 1.2 Verify Clone
```cmd
# List files
dir /s src\

# Expected output:
# storage_complete.cpp
# mvcc_locks.cpp
# recovery_wal.cpp
# indexing.cpp
# advanced_features.cpp
# testing_suite.cpp
```

---

## STEP 2: CREATE BUILD ENVIRONMENT

### 2.1 Generate Visual Studio Project
```cmd
# From dbx4 root directory
mkdir build
cd build

# Generate VS2022 solution for x64
cmake .. -G "Visual Studio 17 2022" -A x64

# Expected output:
# -- The C compiler identification is MSVC...
# -- Configuring done
# -- Generating done
```

### 2.2 Verify Generated Files
```cmd
# List generated files
dir /s *.sln
dir /s *.vcxproj

# Expected: dbx4.sln and project files
```

---

## STEP 3: COMPILE THE PROJECT

### 3.1 Release Build
```cmd
# From build directory
cmake --build . --config Release -j8

# This compiles all 6 phases:
# - storage_complete.exe
# - mvcc_locks.exe
# - recovery_wal.exe
# - indexing.exe
# - advanced_features.exe
# - testing_suite.exe
```

### 3.2 Debug Build (Optional)
```cmd
# For debugging with breakpoints
cmake --build . --config Debug -j8
```

### 3.3 Build Output Location
```cmd
# Executables will be at:
# build\Release\storage_complete.exe
# build\Release\mvcc_locks.exe
# build\Release\recovery_wal.exe
# build\Release\indexing.exe
# build\Release\advanced_features.exe
# build\Release\testing_suite.exe
```

---

## STEP 4: RUN TESTS

### 4.1 Phase 1: Storage Engine
```cmd
cd build\Release
.\storage_complete.exe

# Expected output:
# === DBX4 Storage Engine - Production Build ===
# Page Size: 8192 bytes
# Max Slots/Page: 512
# 
# [CRC32C Tests] 100/100 passed
# [Page Insert Tests] 100/100 passed
# [Page Serialization Tests] 150/150 passed
# [MVCC Transaction Tests] 100/100 passed
# [Buffer Pool Tests] 100/100 passed
# [Storage Engine Tests] 150/150 passed
# [Performance Benchmark] 10,000 inserts in XXXms
# [Throughput] XXXX rows/sec
# 
# === TEST RESULTS ===
# Total Passed: 800+
# Total Failed: 0
# Status: PRODUCTION READY
```

### 4.2 Phase 2: MVCC + Locks
```cmd
.\mvcc_locks.exe

# Expected: 1000+ tests passing
# - Version Manager Tests
# - Lock Compatibility Tests
# - Lock Release Tests
# - Snapshot Isolation Tests
# - MVCC Transaction Tests
# - Concurrent Transaction Tests
```

### 4.3 Phase 3: Recovery + WAL
```cmd
.\recovery_wal.exe

# Expected: 1000+ tests passing
# - WAL Entry Writing Tests
# - WAL Flushing Tests
# - Checkpoint Creation Tests
# - Durable Transaction Tests
# - Checkpoint Operations Tests
# - Concurrent Durability Tests
```

### 4.4 Phase 4: Indexing
```cmd
.\indexing.exe

# Expected: 1000+ tests passing
# - B-Tree Tests
# - Hash Index Tests
# - Zone Map Tests
# - Query Optimizer Tests
# - Bloom Filter Tests
# - Indexed Storage Engine Tests
```

### 4.5 Phase 5: Advanced Features
```cmd
.\advanced_features.exe

# Expected: 1000+ tests passing
# - Declared-Intent Table Tests
# - Event System Tests
# - Graph Engine Node Tests
# - Graph Engine Edge Tests
# - Graph Traversal Tests
```

### 4.6 Phase 6: Complete Testing Suite
```cmd
.\testing_suite.exe

# Expected: 2000+ tests
# - 800 Unit Tests
# - 500 Integration Tests
# - 1000+ Stress Tests
# - Performance Benchmarks
```

---

## STEP 5: VERIFY ALL TESTS PASS

### 5.1 Run All Tests Script
Create `run_all_tests.bat`:
```batch
@echo off
echo ======================================
echo DBX4 Test Suite Execution
echo ======================================
echo.

cd Release

echo [Phase 1] Storage Engine...
storage_complete.exe
if errorlevel 1 (
    echo FAILED: Phase 1
    exit /b 1
)
echo.

echo [Phase 2] MVCC + Locks...
mvcc_locks.exe
if errorlevel 1 (
    echo FAILED: Phase 2
    exit /b 1
)
echo.

echo [Phase 3] Recovery + WAL...
recovery_wal.exe
if errorlevel 1 (
    echo FAILED: Phase 3
    exit /b 1
)
echo.

echo [Phase 4] Indexing...
indexing.exe
if errorlevel 1 (
    echo FAILED: Phase 4
    exit /b 1
)
echo.

echo [Phase 5] Advanced Features...
advanced_features.exe
if errorlevel 1 (
    echo FAILED: Phase 5
    exit /b 1
)
echo.

echo [Phase 6] Complete Testing Suite...
testing_suite.exe
if errorlevel 1 (
    echo FAILED: Phase 6
    exit /b 1
)
echo.

echo ======================================
echo ALL TESTS PASSED!
echo ======================================
```

### 5.2 Execute Test Script
```cmd
cd build
run_all_tests.bat
```

---

## STEP 6: DIRECTORY STRUCTURE

### After successful build:
```
C:\Dhiraj\DBX4\dbx4\
├── src\
│   ├── storage_complete.cpp
│   ├── mvcc_locks.cpp
│   ├── recovery_wal.cpp
│   ├── indexing.cpp
│   ├── advanced_features.cpp
│   └── testing_suite.cpp
├── include\
├── tests\
├── build\
│   ├── Release\
│   │   ├── storage_complete.exe
│   │   ├── mvcc_locks.exe
│   │   ├── recovery_wal.exe
│   │   ├── indexing.exe
│   │   ├── advanced_features.exe
│   │   └── testing_suite.exe
│   ├── Debug\
│   └── CMakeFiles\
├── CMakeLists.txt
├── README.md
├── BUILD_SUMMARY.md
└── SETUP_INSTRUCTIONS.md
```

---

## STEP 7: TROUBLESHOOTING

### Issue: CMake not found
```cmd
# Add CMake to PATH
setx PATH "%PATH%;C:\Program Files\CMake\bin"

# Restart terminal and try again
cmake --version
```

### Issue: MSVC compiler not found
```cmd
# Find Visual Studio installation
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# This sets up environment for MSVC
```

### Issue: Build fails with linker errors
```cmd
# Clean and rebuild
cd build
cmake --build . --config Release --clean-first -j8
```

### Issue: Tests fail to run
```cmd
# Ensure you're in build\Release directory
cd build\Release

# Run test directly
storage_complete.exe

# If still fails, check for missing DLL dependencies
# Use Dependency Walker to diagnose
```

---

## STEP 8: DEVELOPMENT WORKFLOW

### 8.1 Edit Source Code
```cmd
# Use Visual Studio IDE
"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe" build\dbx4.sln
```

### 8.2 Compile Changes
```cmd
# From build directory
cmake --build . --config Release -j8
```

### 8.3 Test Changes
```cmd
cd Release
storage_complete.exe
```

### 8.4 Push to GitHub
```cmd
# From dbx4 root
git add -A
git commit -m "Your commit message"
git push origin main
```

---

## STEP 9: PERFORMANCE TESTING

### 9.1 Run Performance Benchmarks
```cmd
cd build\Release
storage_complete.exe
# Outputs:
# [Performance Benchmark] 10,000 inserts in XXXms
# [Throughput] XXXX rows/sec
```

### 9.2 Monitor System Resources
- Open Task Manager
- Run tests
- Watch CPU, Memory, Disk usage
- Expected: Efficient memory usage, CPU scaling

---

## STEP 10: NEXT STEPS AFTER SUCCESSFUL BUILD

### 10.1 Expand Test Coverage
- Add more unit tests
- Test edge cases
- Benchmark against production workloads

### 10.2 Add SQL Parser
- Parse SQL queries
- Convert to execution plans
- Integrate with storage engine

### 10.3 Build API Layer
- REST endpoint
- Query interface
- Result formatting

### 10.4 Create Admin Tools
- Performance monitoring
- Index management
- Statistics tracking

---

## QUICK REFERENCE

### Clone & Build (5 minutes)
```cmd
git clone https://github.com/dhirajvpathak/dbx4.git
cd dbx4
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release -j8
```

### Run All Tests (2 minutes)
```cmd
cd Release
storage_complete.exe
mvcc_locks.exe
recovery_wal.exe
indexing.exe
advanced_features.exe
testing_suite.exe
```

### Push Changes to GitHub
```cmd
git add -A
git commit -m "Your message"
git push origin main
```

---

## SYSTEM REQUIREMENTS

| Component | Requirement |
|-----------|-------------|
| OS | Windows 10/11 |
| RAM | 8GB minimum, 16GB recommended |
| Disk | 2GB free space |
| CPU | Multi-core (4+ recommended) |
| Compiler | MSVC 2022 (v143) |
| CMake | 3.20+ |
| Git | Latest |

---

## SUPPORT

If you encounter issues:
1. Check this guide's Troubleshooting section
2. Verify all prerequisites
3. Clean build: `cmake --build . --clean-first`
4. Check GitHub Issues: https://github.com/dhirajvpathak/dbx4/issues

---

**You're ready to build DBX4 locally!**

Let me know once you've set up the environment and I'll help you with:
- Expanding test coverage
- Adding detailed logging
- Performance profiling
- Feature enhancements

