#ifndef DBX4_WAL_FORMAT_H
#define DBX4_WAL_FORMAT_H
#include <cstdint>
#include <vector>
#include <string>
namespace dbx4 {
struct WALRecord {
    uint32_t record_type;
    uint32_t txn_id;
    uint64_t lsn;
    uint32_t prev_record_lsn;
    uint32_t data_len;
    uint16_t crc16;
    std::vector<uint8_t> data;
    uint32_t record_crc32;
};
struct WALConfig {
    std::string wal_dir;
    uint32_t max_record_size = 1048576;
    bool enable_checksums = true;
    bool enable_compression = false;
    uint32_t sync_mode = 1;
    uint32_t recovery_mode = 2;
    bool paranoid_checks = true;
};
}
#endif
