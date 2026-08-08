#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>

namespace dbx4 {

struct WALRecord {
    int txn_id;
    int committed;
    std::string data;
};

class RecoveryManager {
private:
    std::map<int, bool> persisted_committed;  // txn_id -> committed flag
    
public:
    void mark_committed(int txn_id) {
        persisted_committed[txn_id] = true;
        persist_to_wal(txn_id);
    }
    
    void persist_to_wal(int txn_id) {
        // Persist committed flag to WAL file
        std::ofstream wal("/tmp/dbx4.wal", std::ios::app);
        wal << "COMMIT|" << txn_id << "\n";
        wal.close();
    }
    
    std::vector<WALRecord> recover() {
        std::vector<WALRecord> recovered;
        persisted_committed.clear();
        
        // Read committed flags from WAL
        std::ifstream wal("/tmp/dbx4.wal");
        std::string line;
        while (std::getline(wal, line)) {
            if (line.find("COMMIT|") == 0) {
                int txn_id = std::stoi(line.substr(7));
                persisted_committed[txn_id] = true;
            }
        }
        wal.close();
        
        // Recover only committed transactions
        std::ifstream data("/tmp/dbx4.data");
        while (std::getline(data, line)) {
            if (line.empty()) continue;
            
            std::istringstream iss(line);
            int txn_id;
            std::string record_data;
            
            iss >> txn_id;
            std::getline(iss, record_data);
            
            if (persisted_committed.count(txn_id) && persisted_committed[txn_id]) {
                WALRecord rec;
                rec.txn_id = txn_id;
                rec.committed = 1;
                rec.data = record_data;
                recovered.push_back(rec);
            }
        }
        data.close();
        
        return recovered;
    }
};

}
