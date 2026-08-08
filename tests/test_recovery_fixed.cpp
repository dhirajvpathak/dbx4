#include <iostream>
#include <fstream>
#include <string>
#include <vector>

struct WALEntry {
    int txn_id;
    int committed;
    std::string data;
    
    bool parse(const std::string& line) {
        size_t pos = line.find('|');
        if (pos == std::string::npos) return false;
        txn_id = std::stoi(line.substr(0, pos));
        
        size_t next_pos = line.find('|', pos + 1);
        if (next_pos == std::string::npos) return false;
        committed = std::stoi(line.substr(pos + 1, next_pos - pos - 1));
        
        data = line.substr(next_pos + 1);
        return true;
    }
    
    std::string serialize() {
        return std::to_string(txn_id) + "|" + std::to_string(committed) + "|" + data;
    }
};

int main() {
    std::cout << "Recovery Architecture Test\n\n";
    
    std::string wal_path = "/tmp/test_fixed_recovery.wal";
    system("rm -f /tmp/test_fixed_recovery.wal");
    
    // Step 1: Write entries
    std::cout << "Step 1: Write committed entry to WAL\n";
    {
        WALEntry e;
        e.txn_id = 1;
        e.committed = 1;
        e.data = "row1:id=1,name=Alice";
        
        std::ofstream wal(wal_path, std::ios::app);
        wal << e.serialize() << "\n";
        wal.close();
        std::cout << "  Written: " << e.serialize() << "\n";
    }
    
    std::cout << "\nStep 2: Write uncommitted entry to WAL\n";
    {
        WALEntry e;
        e.txn_id = 2;
        e.committed = 0;
        e.data = "row2:id=2,name=Bob";
        
        std::ofstream wal(wal_path, std::ios::app);
        wal << e.serialize() << "\n";
        wal.close();
        std::cout << "  Written: " << e.serialize() << "\n";
    }
    
    // Step 3: Recover
    std::cout << "\nStep 3: Restart and recover from WAL\n";
    std::vector<WALEntry> recovered;
    
    {
        std::ifstream wal(wal_path);
        std::string line;
        while (std::getline(wal, line)) {
            if (line.empty()) continue;
            
            WALEntry e;
            if (e.parse(line)) {
                if (e.committed == 1) {
                    recovered.push_back(e);
                    std::cout << "  ✅ Recovered: txn=" << e.txn_id << " " << e.data << "\n";
                } else {
                    std::cout << "  Skipped uncommitted: txn=" << e.txn_id << "\n";
                }
            }
        }
        wal.close();
    }
    
    // Step 4: Verify
    std::cout << "\nStep 4: Verify results\n";
    if (recovered.size() == 1 && recovered[0].txn_id == 1) {
        std::cout << "✅ Recovery PASSED\n";
        std::cout << "  - Committed entry recovered\n";
        std::cout << "  - Uncommitted entry skipped\n";
        return 0;
    } else {
        std::cout << "❌ Recovery FAILED\n";
        return 1;
    }
}
