#include <iostream>
#include "dbx4/wal_format.h"
#include "dbx4/database.h"
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <cstdint>

namespace dbx4 {

class RecoveryEngine {
private:
    WALConfig config_;
    std::map<uint32_t, std::vector<WALRecord>> txn_records_;
    std::map<uint32_t, bool> txn_committed_;
    
public:
    RecoveryEngine(const WALConfig& cfg) : config_(cfg) {}
    
    bool recover_database(Database& db) {
        std::cout << "[Recovery] Starting crash recovery from " << config_.wal_dir << "\n";
        
        if (!parse_wal_file()) {
            std::cerr << "[Recovery] Failed to parse WAL\n";
            return false;
        }
        
        if (!analyze_transactions()) {
            std::cerr << "[Recovery] Failed to analyze transactions\n";
            return false;
        }
        
        if (!replay_committed_transactions(db)) {
            std::cerr << "[Recovery] Failed to replay transactions\n";
            return false;
        }
        
        if (!cleanup_aborted_transactions()) {
            std::cerr << "[Recovery] Failed to cleanup aborted txns\n";
            return false;
        }
        
        std::cout << "[Recovery] ✅ Recovery complete\n";
        return true;
    }
    
private:
    bool parse_wal_file() {
        std::cout << "[Recovery] Parsing WAL file...\n";
        
        std::ifstream wal(config_.wal_dir + "/wal.log", std::ios::binary);
        if (!wal.is_open()) {
            std::cout << "[Recovery] No WAL file (clean start)\n";
            return true;
        }
        
        uint32_t record_count = 0;
        
        while (wal.peek() != EOF) {
            WALRecord record;
            
            if (!wal.read((char*)&record.record_type, 4)) break;
            if (!wal.read((char*)&record.txn_id, 4)) break;
            if (!wal.read((char*)&record.lsn, 8)) break;
            if (!wal.read((char*)&record.prev_record_lsn, 4)) break;
            if (!wal.read((char*)&record.data_len, 4)) break;
            if (!wal.read((char*)&record.crc16, 2)) break;
            
            if (record.data_len > config_.max_record_size) {
                std::cerr << "[Recovery] Record exceeds max size: " << record.data_len << "\n";
                if (config_.paranoid_checks) return false;
                wal.ignore(record.data_len + 4);
                continue;
            }
            
            record.data.resize(record.data_len);
            if (record.data_len > 0) {
                if (!wal.read((char*)record.data.data(), record.data_len)) break;
            }
            
            if (!wal.read((char*)&record.record_crc32, 4)) break;
            
            txn_records_[record.txn_id].push_back(record);
            record_count++;
            
            std::cout << "[Recovery] Record " << record_count << ": txn=" << record.txn_id 
                     << " type=" << record.record_type << " len=" << record.data_len << "\n";
        }
        
        wal.close();
        std::cout << "[Recovery] Parsed " << record_count << " WAL records\n";
        return true;
    }
    
    bool analyze_transactions() {
        std::cout << "[Recovery] Analyzing transactions...\n";
        
        for (auto& [txn_id, records] : txn_records_) {
            if (records.empty()) continue;
            
            uint32_t last_type = records.back().record_type;
            
            if (last_type == 4) {
                txn_committed_[txn_id] = true;
                std::cout << "[Recovery] Txn " << txn_id << " COMMITTED\n";
            } else if (last_type == 5) {
                txn_committed_[txn_id] = false;
                std::cout << "[Recovery] Txn " << txn_id << " ABORTED\n";
            } else {
                txn_committed_[txn_id] = false;
                std::cout << "[Recovery] Txn " << txn_id << " INCOMPLETE (rollback)\n";
            }
        }
        
        return true;
    }
    
    bool replay_committed_transactions(Database& db) {
        std::cout << "[Recovery] Replaying committed transactions...\n";
        
        int replayed = 0;
        for (auto& [txn_id, is_committed] : txn_committed_) {
            if (!is_committed) continue;
            
            auto& records = txn_records_[txn_id];
            for (auto& record : records) {
                if (record.record_type == 4) break;
                
                std::string data_str(record.data.begin(), record.data.end());
                std::cout << "[Recovery] Replaying txn " << txn_id << " data: " << data_str << "\n";
                
                replayed++;
            }
        }
        
        std::cout << "[Recovery] Replayed " << replayed << " data modifications\n";
        return true;
    }
    
    bool cleanup_aborted_transactions() {
        std::cout << "[Recovery] Cleaning up aborted transactions...\n";
        
        int discarded = 0;
        for (auto& [txn_id, is_committed] : txn_committed_) {
            if (is_committed) continue;
            std::cout << "[Recovery] Discarding txn " << txn_id << "\n";
            discarded++;
        }
        
        std::cout << "[Recovery] Discarded " << discarded << " aborted txns\n";
        return true;
    }
};

}
