# DBX4 Production Deployment Guide

## Prerequisites

- GCC 14+ or Clang 15+
- C++17 compiler support
- Linux/Unix operating system
- Minimum 4GB RAM recommended

## Installation

### 1. Build from Source

```bash
git clone https://github.com/dhirajvpathak/dbx4.git
cd dbx4
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j4
```

### 2. Verify Build

```bash
# Run test suite
ctest --verbose

# Should see:
# ✅ Concurrency test PASSED
# ✅ Storage test PASSED
# ✅ Transaction test PASSED
# ✅ Recovery test PASSED
# ✅ SQL test PASSED
```

### 3. Install

```bash
cmake --install .
# Installs to /usr/local/lib/libdbx4.a
# and /usr/local/include/dbx4/
```

## Configuration

### Environment Variables

```bash
export DBX4_DATA_DIR=/var/lib/dbx4          # Data directory
export DBX4_WAL_DIR=/var/log/dbx4           # WAL directory
export DBX4_CACHE_SIZE=1000                 # Max cached rows
export DBX4_MAX_CONNECTIONS=100             # Max concurrent clients
export DBX4_LOG_LEVEL=INFO                  # DEBUG|INFO|WARN|ERROR
```

### File Permissions

```bash
mkdir -p /var/lib/dbx4 /var/log/dbx4
chmod 700 /var/lib/dbx4 /var/log/dbx4
chown dbx4:dbx4 /var/lib/dbx4 /var/log/dbx4
```

## Monitoring

### Health Check

```bash
./dbx4_admin health_check
```

Expected output:
```
Memory: 124 MB
Cache hit rate: 85%
Active txns: 3
Error rate: 0.00%
Status: HEALTHY
```

### Metrics Collection

```bash
# Enable metrics export
./dbx4_admin metrics --format prometheus --output /var/log/dbx4/metrics.prom
```

## Backup Strategy

### Daily Backup

```bash
#!/bin/bash
DATE=$(date +%Y%m%d)
BACKUP_DIR=/backups/dbx4/$DATE

mkdir -p $BACKUP_DIR
./dbx4_admin backup --output $BACKUP_DIR
gzip $BACKUP_DIR/*

# Verify
./dbx4_admin verify_backup --path $BACKUP_DIR
```

### Retention Policy

- Daily backups: Keep 30 days
- Weekly backups: Keep 12 weeks
- Monthly backups: Keep 12 months

## Operations

### Starting the Service

```bash
systemctl start dbx4
systemctl status dbx4
```

### Graceful Shutdown

```bash
./dbx4_admin shutdown --wait 30
# Waits 30 seconds for transactions to complete
```

### Emergency Stop

```bash
kill -TERM $(cat /var/run/dbx4.pid)
```

