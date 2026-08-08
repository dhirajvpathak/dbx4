# DBX4 Troubleshooting Guide

## Common Issues and Solutions

### Issue: High Memory Usage

**Symptom:** Memory usage growing unbounded

**Diagnosis:**
```bash
./dbx4_admin show_metrics | grep Memory
./dbx4_admin show_cache_stats
```

**Solutions:**
1. Reduce cache size: `export DBX4_CACHE_SIZE=500`
2. Check for memory leaks: Run with ASAN
3. Monitor WAL file size

### Issue: Slow Queries

**Symptom:** SELECT queries taking >100ms

**Diagnosis:**
```bash
./dbx4_admin profile_queries --threshold 100
```

**Solutions:**
1. Add indexes on WHERE clause columns
2. Use LIMIT to reduce result set
3. Check for lock contention: `./dbx4_admin show_locks`

### Issue: Recovery Takes Too Long

**Symptom:** Database startup time > 5 minutes

**Diagnosis:**
```bash
./dbx4_admin analyze_wal --show-size
```

**Solutions:**
1. Compact WAL: `./dbx4_admin compact_wal`
2. Archive old WAL files
3. Reduce number of small transactions

### Issue: Data Corruption Detected

**Symptom:** Checksum mismatch error

**Diagnosis:**
```bash
./dbx4_admin verify_database --report corrupted_pages.txt
```

**Solutions:**
1. Restore from backup: See Deployment Guide
2. Use point-in-time recovery
3. Run manual recovery script

## Debug Procedures

### Enable Debug Logging

```bash
export DBX4_LOG_LEVEL=DEBUG
./dbx4_server 2>&1 | tee dbx4_debug.log
```

### Crash Dump Analysis

```bash
# Generate core dump
ulimit -c unlimited

# Run with gdb
gdb ./dbx4_server ./core
(gdb) bt  # Backtrace
(gdb) info locals
```

### Memory Leak Detection

```bash
valgrind --leak-check=full --show-leak-kinds=all ./dbx4_server
```

