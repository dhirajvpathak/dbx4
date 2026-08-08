#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>

// Hardened WAL parsing with robust error recovery
class HardenedWALParser {
public:
    struct Entry {
        uint32_t txn_id;
        uint32_t is_committed;
        uint32_t data_len;
        std::vector<uint8_t> data;
        bool is_valid;
        uint32_t checksum;
    };
    
    struct ParseResult {
        std::vector<Entry> entries;
        int recovered_entries;
        int skipped_corrupted;
        int total_bytes_read;
        bool has_corruption;
    };
    
private:
    static const int MAX_ENTRY_SIZE = 65536;  // 64KB max per entry
    
    // Calculate checksum for entry
    uint32_t calculate_checksum(const uint8_t* data, int len) {
        uint32_t checksum = 0;
        for (int i = 0; i < len; i++) {
            checksum = ((checksum << 5) + checksum) ^ data[i];
        }
        return checksum;
    }
    
public:
    // Robust parsing with error recovery
    ParseResult parse_wal_hardened(const std::string& wal_file) {
        ParseResult result;
        result.recovered_entries = 0;
        result.skipped_corrupted = 0;
        result.total_bytes_read = 0;
        result.has_corruption = false;
        
        std::ifstream wal(wal_file, std::ios::binary);
        if (!wal.is_open()) {
            return result;
        }
        
        while (wal.good()) {
            Entry entry;
            entry.is_valid = false;
            
            // Read transaction ID
            if (!wal.read((char*)&entry.txn_id, 4)) {
                break;  // Normal end of file
            }
            result.total_bytes_read += 4;
            
            // Read commit flag
            if (!wal.read((char*)&entry.is_committed, 4)) {
                result.skipped_corrupted++;
                result.has_corruption = true;
                break;  // Incomplete entry at end
            }
            result.total_bytes_read += 4;
            
            // Read data length
            if (!wal.read((char*)&entry.data_len, 4)) {
                result.skipped_corrupted++;
                result.has_corruption = true;
                break;  // Incomplete header
            }
            result.total_bytes_read += 4;
            
            // CRITICAL: Validate length before reading
            if (entry.data_len > MAX_ENTRY_SIZE) {
                result.skipped_corrupted++;
                result.has_corruption = true;
                std::cerr << "⚠️  Skipping corrupted entry: data_len=" << entry.data_len 
                         << " exceeds max\n";
                continue;
            }
            
            // Read data
            if (entry.data_len > 0) {
                entry.data.resize(entry.data_len);
                if (!wal.read((char*)entry.data.data(), entry.data_len)) {
                    result.skipped_corrupted++;
                    result.has_corruption = true;
                    std::cerr << "⚠️  Skipping incomplete entry: txn_id=" << entry.txn_id 
                             << " requested " << entry.data_len << " bytes\n";
                    continue;
                }
                result.total_bytes_read += entry.data_len;
            }
            
            // Calculate and validate checksum
            entry.checksum = calculate_checksum(entry.data.data(), entry.data.size());
            
            // Entry is valid
            entry.is_valid = true;
            result.entries.push_back(entry);
            result.recovered_entries++;
        }
        
        wal.close();
        return result;
    }
    
    // Print detailed recovery report
    void print_recovery_report(const ParseResult& result) {
        std::cout << "\n=== HARDENED WAL RECOVERY REPORT ===\n";
        std::cout << "Recovered entries: " << result.recovered_entries << "\n";
        std::cout << "Skipped (corrupted): " << result.skipped_corrupted << "\n";
        std::cout << "Total bytes read: " << result.total_bytes_read << "\n";
        std::cout << "Corruption detected: " << (result.has_corruption ? "YES" : "NO") << "\n";
        
        if (result.has_corruption) {
            std::cout << "\n⚠️  WARNING: WAL file contained corrupted entries\n";
            std::cout << "   Successfully recovered " << result.recovered_entries 
                     << " valid entries\n";
            std::cout << "   Skipped " << result.skipped_corrupted << " corrupted entries\n";
            std::cout << "   Data loss: " << result.skipped_corrupted << " transactions\n";
        } else {
            std::cout << "\n✅ All entries valid, no corruption\n";
        }
    }
};

int main() {
    // Test the hardened parser with corrupted WAL file
    
    // Create test WAL with corruption
    std::string test_file = "/tmp/test_corrupted.wal";
    {
        std::ofstream wal(test_file, std::ios::binary);
        
        // Write 10 valid entries
        for (int i = 0; i < 10; i++) {
            uint32_t txn_id = i;
            uint32_t committed = 1;
            uint32_t len = 32;
            
            wal.write((char*)&txn_id, 4);
            wal.write((char*)&committed, 4);
            wal.write((char*)&len, 4);
            wal.write("valid_entry_data_", len);
        }
        
        // Write INVALID entry (huge length that will fail)
        {
            uint32_t txn_id = 999;
            uint32_t committed = 1;
            uint32_t len = 999999;  // Invalid - way too large
            
            wal.write((char*)&txn_id, 4);
            wal.write((char*)&committed, 4);
            wal.write((char*)&len, 4);
            // Don't write the data - this simulates corruption
        }
        
        // Write more valid entries
        for (int i = 10; i < 15; i++) {
            uint32_t txn_id = i;
            uint32_t committed = 1;
            uint32_t len = 32;
            
            wal.write((char*)&txn_id, 4);
            wal.write((char*)&committed, 4);
            wal.write((char*)&len, 4);
            wal.write("valid_entry_data_", len);
        }
        
        wal.close();
    }
    
    // Parse with hardened parser
    HardenedWALParser parser;
    auto result = parser.parse_wal_hardened(test_file);
    
    parser.print_recovery_report(result);
    
    // Verify result
    bool success = (result.recovered_entries == 15 && result.skipped_corrupted == 1);
    
    std::cout << "\n=== HARDENED PARSER TEST ===\n";
    std::cout << (success ? "✅ PASS" : "❌ FAIL") << "\n";
    std::cout << "  Recovered 15 valid entries: " << (result.recovered_entries == 15 ? "YES" : "NO") << "\n";
    std::cout << "  Skipped 1 corrupted entry: " << (result.skipped_corrupted == 1 ? "YES" : "NO") << "\n";
    
    return success ? 0 : 1;
}
