#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <cstring>
#include <cassert>

// Robust WAL Codec - preserves all bytes including NUL, |, newlines

class RobustWALCodec {
public:
    static const int MAX_ENTRY_SIZE = 65536;
    
    struct WALEntry {
        uint32_t txn_id;
        uint32_t data_len;
        std::vector<uint8_t> data;
    };
    
    // Write entry: preserves ALL bytes, no truncation
    static bool write_entry(std::ofstream& file, uint32_t txn_id, 
                           const std::vector<uint8_t>& data) {
        if (data.size() > MAX_ENTRY_SIZE) {
            return false;
        }
        
        uint32_t len = data.size();
        
        // Write: txn_id(4) + data_len(4) + data(N)
        file.write((char*)&txn_id, 4);
        file.write((char*)&len, 4);
        
        if (len > 0) {
            file.write((char*)data.data(), len);
        }
        
        return true;
    }
    
    // Read entry: recovers ALL bytes exactly
    static bool read_entry(std::ifstream& file, WALEntry& entry) {
        uint32_t txn_id, data_len;
        
        if (!file.read((char*)&txn_id, 4)) return false;
        if (!file.read((char*)&data_len, 4)) return false;
        
        // Validate length
        if (data_len > MAX_ENTRY_SIZE) {
            return false;  // Corrupted
        }
        
        entry.txn_id = txn_id;
        entry.data_len = data_len;
        
        if (data_len > 0) {
            entry.data.resize(data_len);
            if (!file.read((char*)entry.data.data(), data_len)) {
                return false;  // Incomplete
            }
        }
        
        return true;
    }
    
    // Verify round-trip preserves all bytes
    static bool verify_roundtrip(const std::vector<uint8_t>& original,
                                const std::vector<uint8_t>& recovered) {
        if (original.size() != recovered.size()) {
            return false;
        }
        
        for (size_t i = 0; i < original.size(); i++) {
            if (original[i] != recovered[i]) {
                return false;
            }
        }
        
        return true;
    }
};

// Test the codec
int main() {
    std::cout << "Robust WAL Codec Test\n";
    std::cout << "====================\n\n";
    
    // Test case 1: Special characters including NUL
    std::vector<uint8_t> test_data = {
        'A', '|', 'B', '\n', 'C', '\0', 'D'  // Pipe, newline, NUL, more data
    };
    
    std::cout << "Test 1: Data with | , newline, embedded NUL\n";
    std::cout << "  Original: " << test_data.size() << " bytes\n";
    
    // Write
    std::ofstream wal("/tmp/wal_robust_test.log", std::ios::binary);
    RobustWALCodec::write_entry(wal, 1, test_data);
    wal.close();
    
    // Read
    std::ifstream verify("/tmp/wal_robust_test.log", std::ios::binary);
    RobustWALCodec::WALEntry entry;
    RobustWALCodec::read_entry(verify, entry);
    verify.close();
    
    bool match = RobustWALCodec::verify_roundtrip(test_data, entry.data);
    
    std::cout << "  Read: " << entry.data_len << " bytes\n";
    std::cout << "  Match: " << (match ? "✅ PASS" : "❌ FAIL") << "\n";
    
    // Test case 2: Binary data
    std::vector<uint8_t> binary_data;
    for (int i = 0; i < 256; i++) {
        binary_data.push_back((uint8_t)i);  // All possible byte values
    }
    
    std::cout << "\nTest 2: All possible byte values (0-255)\n";
    std::cout << "  Original: " << binary_data.size() << " bytes\n";
    
    wal.open("/tmp/wal_binary_test.log", std::ios::binary);
    RobustWALCodec::write_entry(wal, 2, binary_data);
    wal.close();
    
    verify.open("/tmp/wal_binary_test.log", std::ios::binary);
    RobustWALCodec::read_entry(verify, entry);
    verify.close();
    
    bool binary_match = RobustWALCodec::verify_roundtrip(binary_data, entry.data);
    
    std::cout << "  Read: " << entry.data_len << " bytes\n";
    std::cout << "  Match: " << (binary_match ? "✅ PASS" : "❌ FAIL") << "\n";
    
    return (match && binary_match) ? 0 : 1;
}
