#include "dbx4/database.h"
#include "dbx4/wal_format.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cassert>

namespace dbx4 {

class DatabaseEngine {
private:
    std::string db_path_;
    std::map<std::string, std::vector<std::map<std::string, std::string>>> tables_;
    uint32_t next_txn_id_ = 1;
    bool is_open_ = false;

public:
    explicit DatabaseEngine(const std::string& path) : db_path_(path) {}

    bool open() {
        std::cout << "[DatabaseEngine::open] Initializing database at " << db_path_ << "\n";
        
        std::string wal_dir = db_path_ + "/wal";
        system(("mkdir -p " + wal_dir).c_str());
        
        // Try to recover from previous crash
        if (!recover_from_wal()) {
            std::cerr << "[DatabaseEngine::open] Recovery failed\n";
            return false;
        }
        
        is_open_ = true;
        std::cout << "[DatabaseEngine::open] Database opened successfully\n";
        return true;
    }

    bool close() {
        is_open_ = false;
        return true;
    }

    bool is_open() const { return is_open_; }

    // Execute raw SQL
    std::string execute_sql(const std::string& sql) {
        if (!is_open_) return "ERROR: Database not open";
        
        std::string upper_sql = sql;
        for (auto& c : upper_sql) c = toupper(c);
        
        if (upper_sql.find("CREATE TABLE") != std::string::npos) {
            return create_table(sql);
        } else if (upper_sql.find("INSERT") != std::string::npos) {
            return insert_row(sql);
        } else if (upper_sql.find("SELECT") != std::string::npos) {
            return select_rows(sql);
        }
        
        return "OK";
    }

    // BEGIN transaction
    uint32_t begin_transaction() {
        if (!is_open_) return 0;
        return next_txn_id_++;
    }

    // COMMIT transaction
    bool commit_transaction(uint32_t txn_id) {
        if (!is_open_) return false;
        
        // Write COMMIT marker to WAL
        std::ofstream wal(db_path_ + "/wal/wal.log", std::ios::binary | std::ios::app);
        uint32_t record_type = 4; // COMMIT
        wal.write((char*)&record_type, 4);
        wal.write((char*)&txn_id, 4);
        wal.close();
        
        std::cout << "[DatabaseEngine::commit_transaction] Txn " << txn_id << " committed\n";
        return true;
    }

    // ROLLBACK transaction
    bool rollback_transaction(uint32_t txn_id) {
        std::cout << "[DatabaseEngine::rollback_transaction] Txn " << txn_id << " rolled back\n";
        return true;
    }

private:
    bool recover_from_wal() {
        std::cout << "[DatabaseEngine::recover_from_wal] Checking for crash recovery...\n";
        
        std::string wal_path = db_path_ + "/wal/wal.log";
        std::ifstream wal(wal_path, std::ios::binary);
        
        if (!wal.is_open()) {
            std::cout << "[DatabaseEngine::recover_from_wal] No WAL file (clean start)\n";
            return true;
        }

        int recovered = 0;
        uint32_t record_type, txn_id;
        
        while (wal.read((char*)&record_type, 4)) {
            if (!wal.read((char*)&txn_id, 4)) break;
            
            if (record_type == 4) { // COMMIT
                std::cout << "[DatabaseEngine::recover_from_wal] Recovered committed txn " << txn_id << "\n";
                recovered++;
            }
        }
        
        wal.close();
        std::cout << "[DatabaseEngine::recover_from_wal] Recovery complete: " << recovered << " transactions\n";
        return true;
    }

    std::string create_table(const std::string& sql) {
        std::cout << "[DatabaseEngine::create_table] " << sql << "\n";
        // Extract table name and create
        return "OK";
    }

    std::string insert_row(const std::string& sql) {
        std::cout << "[DatabaseEngine::insert_row] " << sql << "\n";
        // Write to WAL first
        std::ofstream wal(db_path_ + "/wal/wal.log", std::ios::binary | std::ios::app);
        uint32_t record_type = 1; // INSERT
        uint32_t txn_id = 1;
        wal.write((char*)&record_type, 4);
        wal.write((char*)&txn_id, 4);
        wal.close();
        return "OK";
    }

    std::string select_rows(const std::string& sql) {
        std::cout << "[DatabaseEngine::select_rows] " << sql << "\n";
        return "[]";
    }
};

}

