#ifndef DBX4_TRANSACTION_LOG_H
#define DBX4_TRANSACTION_LOG_H
#include "dbx4/wal_format.h"
#include <fstream>
#include <ctime>
#include <cstdint>
namespace dbx4 {
class TransactionLog {
private:
    std::string wal_path_;
    std::ofstream wal_file_;
    uint64_t current_lsn_ = 0;
public:
    TransactionLog(const std::string& wal_dir) : wal_path_(wal_dir + "/wal.log") {}
    bool open() {
        wal_file_.open(wal_path_, std::ios::binary | std::ios::app);
        return wal_file_.is_open();
    }
    bool write_record(uint32_t txn_id, uint32_t record_type, const std::vector<uint8_t>& data) {
        if (!wal_file_.is_open()) return false;
        WALRecord rec;
        rec.record_type = record_type;
        rec.txn_id = txn_id;
        rec.lsn = current_lsn_;
        rec.prev_record_lsn = current_lsn_ > 0 ? current_lsn_ - 1 : 0;
        rec.data_len = data.size();
        rec.crc16 = 0;
        rec.data = data;
        rec.record_crc32 = 0;
        wal_file_.write((char*)&rec.record_type, 4);
        wal_file_.write((char*)&rec.txn_id, 4);
        wal_file_.write((char*)&rec.lsn, 8);
        wal_file_.write((char*)&rec.prev_record_lsn, 4);
        wal_file_.write((char*)&rec.data_len, 4);
        wal_file_.write((char*)&rec.crc16, 2);
        if (rec.data_len > 0) wal_file_.write((char*)rec.data.data(), rec.data_len);
        wal_file_.write((char*)&rec.record_crc32, 4);
        wal_file_.flush();
        current_lsn_ += (24 + rec.data_len);
        return true;
    }
    bool close() {
        if (wal_file_.is_open()) wal_file_.close();
        return true;
    }
};
}
#endif
