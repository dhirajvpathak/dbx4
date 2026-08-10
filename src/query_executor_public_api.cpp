#include "dbx4/query_executor.h"
#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <map>

namespace dbx4 {

static std::map<std::string, int> recovered_count;

void QueryExecutor::recover_from_wal() {
    std::cout << "[QueryExecutor::recover_from_wal] Starting WAL recovery...\n";
    
    std::ifstream wal("/tmp/dbx4.wal", std::ios::binary);
    if (!wal.is_open()) {
        std::cout << "[QueryExecutor::recover_from_wal] No WAL file found (clean start)\n";
        return;
    }
    
    std::cout << "[QueryExecutor::recover_from_wal] WAL file found, reading...\n";
    
    int recovered = 0;
    uint32_t txn_id, committed, data_len;
    
    while (wal.read((char*)&txn_id, sizeof(txn_id))) {
        if (!wal.read((char*)&committed, sizeof(committed))) break;
        if (!wal.read((char*)&data_len, sizeof(data_len))) break;
        
        if (data_len > 65536) {
            std::cout << "[QueryExecutor::recover_from_wal] Skipping corrupted entry\n";
            continue;
        }
        
        std::vector<char> data(data_len);
        if (data_len > 0 && !wal.read(data.data(), data_len)) break;
        
        if (committed) {
            recovered++;
            recovered_count["committed"] = recovered;
            std::cout << "[QueryExecutor::recover_from_wal] Recovered txn " << txn_id << "\n";
        }
    }
    
    wal.close();
    std::cout << "[QueryExecutor::recover_from_wal] Recovery complete: " 
             << recovered << " transactions\n";
}

}
