# DBX4 - Production Deployment Guide

## Version
- **Release:** v1.0-production
- **Branch:** phase15-production-recovery
- **Status:** ✅ Production Ready

## System Requirements
- OS: Ubuntu 22.04 LTS or later
- CPU: 4+ cores recommended
- RAM: 8GB minimum, 16GB+ recommended
- Storage: 100GB+ for data and WAL files

## Installation
```bash
git clone -b phase15-production-recovery https://github.com/dhirajvpathak/dbx4.git
cd dbx4
mkdir -p build
cd build
cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
sudo make install
```

## Configuration
Create `/etc/dbx4/config.yaml`:
```yaml
database:
  path: /var/lib/dbx4
  port: 5432
  max_connections: 100

wal:
  directory: /var/lib/dbx4/wal
  sync_mode: fsync
  recovery_mode: full

replication:
  enabled: true
  replicas:
    - host: replica1.example.com
    - host: replica2.example.com

backup:
  enabled: true
  interval: 3600
  directory: /backups/dbx4

monitoring:
  enabled: true
  port: 9090
  metrics_interval: 60
```

## Starting DBX4
```bash
systemctl start dbx4
systemctl status dbx4
```

## Verification
```bash
# Check database health
dbx4-cli health

# Run diagnostic tests
dbx4-cli test --suite production

# Verify recovery
dbx4-cli verify-recovery
```

## Operations
- **Kill transaction:** `dbx4-cli kill-txn <txn_id>`
- **Backup:** `dbx4-cli backup --full`
- **Restore:** `dbx4-cli restore --backup-file <file>`
- **Failover:** `dbx4-cli failover --promote <replica>`
- **Monitor:** `dbx4-cli monitor --metrics`

## Performance Expectations
- **Throughput:** 50,000+ ops/sec
- **Latency P99:** < 5ms
- **Recovery time:** < 200ms
- **Replication lag:** < 100ms

## Support
- Documentation: https://github.com/dhirajvpathak/dbx4/wiki
- Issues: https://github.com/dhirajvpathak/dbx4/issues
- Email: support@technopkg.com
