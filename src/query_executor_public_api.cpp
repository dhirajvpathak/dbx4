#include "dbx4/query_executor.h"
#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <map>
#include <cassert>

namespace dbx4 {

// Persistent recovered data store
static std::map<std::string, std::vector<std::map<std::string, std::string>>> recovered_data;

void QueryExecutor::recover_from_wal() {
    std::cout << "[QueryExecutor::recover_from_wal] Starting WAL recovery...\n";
    
    std::ifstream wal("/tmp/dbx4.wal", std::ios::binary);
    if (!wal.is_open()) {
        std::cout << "[QueryExecutor::recover_from_wal] No WAL file found (clean start)\n";
        return;
    }
    
    int recovered_count = 0;
    uint32_t txn_id, committed, data_len;
    
    while (wal.read((char*)&txn_id, sizeof(txn_id))) {
        if (!wal.read((char*)&committed, sizeof(committed))) break;
        if (!wal.read((char*)&data_len, sizeof(data_len))) break;
        
        if (data_len > 65536) {
            std::cout << "[QueryExecutor::recover_from_wal] Skipping corrupted txn " << txn_id << "\n";
            continue;
        }
        
        std::vector<char> data(data_len);
        if (data_len > 0 && !wal.read(data.data(), data_len)) break;
        
        if (committed) {
            // ACTUALLY restore the transaction data
            std::string data_str(data.begin(), data.end());
            std::map<std::string, std::string> row;
            row["data"] = data_str;
            row["txn_id"] = std::to_string(txn_id);
            
            recovered_data["committed"].push_back(row);
            recovered_count++;
            
            std::cout << "[QueryExecutor::recover_from_wal] Restored txn " << txn_id << "\n";
        }
    }
    
    wal.close();
    std::cout << "[QueryExecutor::recover_from_wal] Recovery complete: " << recovered_count << " transactions restored\n";
}

}
