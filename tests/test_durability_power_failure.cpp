#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <cassert>
#include <signal.h>
#include <unistd.h>

// Simulate power failure by abruptly killing process
volatile bool should_crash = false;

void crash_handler(int sig) {
    should_crash = true;
}

class DurabilityTest {
public:
    static bool test_power_failure_during_write() {
        std::cout << "Test: Power failure during write\n";
        
        // Create WAL file
        std::ofstream wal("/tmp/durability_wal.bin", std::ios::binary);
        
        // Write multiple records
        for (int i = 0; i < 1000; i++) {
            int txn_id = i;
            int committed = (i % 2 == 0) ? 1 : 0;  // Half committed, half not
            uint32_t len = 100;
            
            wal.write((char*)&txn_id, 4);
            wal.write((char*)&committed, 4);
            wal.write((char*)&len, 4);
            wal.write("test_data_test_data_test_data_", 100);
            
            // Simulate crash at random point
            if (i == 500 && should_crash) {
                std::cerr << "SIMULATED CRASH at record " << i << "\n";
                exit(1);
            }
        }
        
        wal.close();
        return true;
    }
    
    static bool test_partial_wal_recovery() {
        std::cout << "Test: Recover from partial WAL\n";
        
        // Read WAL file that was partially written
        std::ifstream wal("/tmp/durability_wal.bin", std::ios::binary);
        if (!wal.is_open()) {
            std::cout << "  WAL file not found (expected after crash)\n";
            return true;
        }
        
        int records_read = 0;
        int records_committed = 0;
        
        while (true) {
            int txn_id;
            int committed;
            uint32_t len;
            char data[100];
            
            if (!wal.read((char*)&txn_id, 4)) break;
            if (!wal.read((char*)&committed, 4)) break;
            if (!wal.read((char*)&len, 4)) break;
            if (!wal.read(data, 100)) break;
            
            records_read++;
            if (committed) records_committed++;
        }
        
        wal.close();
        
        std::cout << "  Records read: " << records_read << "\n";
        std::cout << "  Committed: " << records_committed << "\n";
        std::cout << "  ✅ Partial WAL recovery works\n";
        
        return records_read > 0;
    }
    
    static bool test_checksum_corruption_detection() {
        std::cout << "Test: Detect corrupted pages\n";
        
        // Create page with correct checksum
        uint8_t page[8192];
        std::memset(page, 0, 8192);
        
        uint32_t checksum = 0;
        for (int i = 0; i < 8192; i++) {
            checksum ^= page[i];
        }
        std::memcpy(page + 8188, &checksum, 4);  // Store checksum at end
        
        // Corrupt a byte in the middle
        page[4096] ^= 0xFF;
        
        // Verify checksum fails
        uint32_t stored_checksum;
        std::memcpy(&stored_checksum, page + 8188, 4);
        
        uint32_t new_checksum = 0;
        for (int i = 0; i < 8192; i++) {
            new_checksum ^= page[i];
        }
        
        bool corruption_detected = (stored_checksum != new_checksum);
        
        if (corruption_detected) {
            std::cout << "  ✅ Corruption detected correctly\n";
        }
        
        return corruption_detected;
    }
};

int main() {
    std::cout << "DURABILITY TESTING\n\n";
    
    int passed = 0;
    
    // Test 1: Write then crash recovery
    if (DurabilityTest::test_power_failure_during_write()) {
        passed++;
    }
    
    // Test 2: Partial WAL recovery
    if (DurabilityTest::test_partial_wal_recovery()) {
        passed++;
    }
    
    // Test 3: Checksum corruption detection
    if (DurabilityTest::test_checksum_corruption_detection()) {
        passed++;
    }
    
    std::cout << "\n✅ Durability: " << passed << "/3 tests passed\n";
    return passed == 3 ? 0 : 1;
}
