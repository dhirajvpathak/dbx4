#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cstring>

namespace dbx4 {

struct WALRecord {
    int txn_id;
    int committed;
    std::string data;
    
    // Length-prefixed encoding (not lossy delimiter-based)
    std::string serialize() const {
        std::string result;
        // txn_id (4 bytes)
        result.append(const_cast<char*>(reinterpret_cast<const char*>(&txn_id)), 4);
        // committed (4 bytes)
        result.append(const_cast<char*>(reinterpret_cast<const char*>(&committed)), 4);
        // data length (4 bytes)
        uint32_t len = data.length();
        result.append(const_cast<char*>(reinterpret_cast<const char*>(&len)), 4);
        // data (variable)
        result.append(data);
        return result;
    }
    
    static WALRecord deserialize(const std::string& serialized) {
        WALRecord rec;
        if (serialized.size() < 12) return rec;
        
        std::memcpy(&rec.txn_id, serialized.data(), 4);
        std::memcpy(&rec.committed, serialized.data() + 4, 4);
        uint32_t len;
        std::memcpy(&len, serialized.data() + 8, 4);
        
        if (serialized.size() < 12 + len) return rec;
        rec.data = serialized.substr(12, len);
        
        return rec;
    }
};

class WALManager {
private:
    std::string wal_path;
    std::map<int, bool> persisted_committed;
    
public:
    WALManager(const std::string& path) : wal_path(path) {}
    
    void write_record(const WALRecord& record) {
        std::ofstream wal(wal_path, std::ios::binary | std::ios::app);
        std::string serialized = record.serialize();
        wal.write(serialized.c_str(), serialized.length());
        wal.close();
    }
    
    void mark_committed(int txn_id) {
        persisted_committed[txn_id] = true;
        
        // Write COMMIT marker to WAL
        WALRecord commit_rec;
        commit_rec.txn_id = txn_id;
        commit_rec.committed = 1;
        commit_rec.data = "COMMIT";
        write_record(commit_rec);
    }
    
    std::vector<WALRecord> recover() {
        std::vector<WALRecord> recovered;
        persisted_committed.clear();
        
        // First pass: read all commit markers
        std::ifstream wal(wal_path, std::ios::binary);
        if (!wal.is_open()) return recovered;
        
        std::string buffer((std::istreambuf_iterator<char>(wal)),
                          std::istreambuf_iterator<char>());
        wal.close();
        
        // Parse records
        size_t pos = 0;
        while (pos < buffer.length()) {
            if (pos + 12 > buffer.length()) break;
            
            uint32_t len;
            std::memcpy(&len, buffer.data() + pos + 8, 4);
            
            if (pos + 12 + len > buffer.length()) break;
            
            std::string rec_data = buffer.substr(pos, 12 + len);
            WALRecord rec = WALRecord::deserialize(rec_data);
            
            if (rec.committed == 1) {
                persisted_committed[rec.txn_id] = true;
            }
            
            pos += 12 + len;
        }
        
        // Second pass: recover committed records only
        pos = 0;
        while (pos < buffer.length()) {
            if (pos + 12 > buffer.length()) break;
            
            uint32_t len;
            std::memcpy(&len, buffer.data() + pos + 8, 4);
            
            if (pos + 12 + len > buffer.length()) break;
            
            std::string rec_data = buffer.substr(pos, 12 + len);
            WALRecord rec = WALRecord::deserialize(rec_data);
            
            if (persisted_committed.count(rec.txn_id)) {
                recovered.push_back(rec);
            }
            
            pos += 12 + len;
        }
        
        return recovered;
    }
};

}
