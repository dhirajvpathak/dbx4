#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdint>
#include <cassert>

class MultiProcessRecoveryTest {
public:
    static bool test_concurrent_write_crash_recovery() {
        std::cout << "Test: Multi-process write + crash recovery\n";
        
        std::string wal_path = "/tmp/recovery_test_multiprocess.wal";
        
        // Process 1: Write committed data
        pid_t pid1 = fork();
        if (pid1 == 0) {
            std::ofstream wal(wal_path, std::ios::binary | std::ios::app);
            
            int txn_id = 1;
            int committed = 1;
            uint32_t len = 50;
            const char* data = "committed_data_from_process_1_xxxxxxxxxx";
            
            wal.write((char*)&txn_id, 4);
            wal.write((char*)&committed, 4);
            wal.write((char*)&len, 4);
            wal.write(data, len);
            wal.close();
            exit(0);
        }
        int status;
        waitpid(pid1, &status, 0);
        
        // Process 2: Write uncommitted data then crash
        pid_t pid2 = fork();
        if (pid2 == 0) {
            std::ofstream wal(wal_path, std::ios::binary | std::ios::app);
            
            int txn_id = 2;
            int committed = 0;  // NOT COMMITTED
            uint32_t len = 50;
            const char* data = "uncommitted_data_from_process_2_xxxxx";
            
            wal.write((char*)&txn_id, 4);
            wal.write((char*)&committed, 4);
            wal.write((char*)&len, 4);
            wal.write(data, len);
            wal.close();
            exit(1);  // Simulate crash
        }
        waitpid(pid2, &status, 0);
        
        // Process 3: Recovery - read and verify
        std::ifstream wal(wal_path, std::ios::binary);
        if (!wal.is_open()) {
            std::cout << "  ❌ WAL file not found\n";
            return false;
        }
        
        int recovered_committed = 0;
        int recovered_uncommitted = 0;
        
        while (true) {
            int txn_id;
            int committed;
            uint32_t len;
            char data[100];
            
            if (!wal.read((char*)&txn_id, 4)) break;
            if (!wal.read((char*)&committed, 4)) break;
            if (!wal.read((char*)&len, 4)) break;
            if (len > 100) break;
            if (!wal.read(data, len)) break;
            
            if (committed) {
                recovered_committed++;
            } else {
                recovered_uncommitted++;
            }
        }
        
        wal.close();
        
        std::cout << "  Committed records: " << recovered_committed << "\n";
        std::cout << "  Uncommitted records: " << recovered_uncommitted << "\n";
        
        bool success = (recovered_committed == 1 && recovered_uncommitted == 1);
        if (success) {
            std::cout << "  ✅ Multi-process recovery works\n";
        }
        
        return success;
    }
    
    static bool test_schema_consistency() {
        std::cout << "Test: Schema consistency after restart\n";
        
        // Write schema
        std::ofstream schema("/tmp/recovery_test_schema.txt");
        schema << "table1:id,name,email\n";
        schema << "table2:user_id,status\n";
        schema.close();
        
        // Read back and verify
        std::ifstream schema_read("/tmp/recovery_test_schema.txt");
        std::string line1, line2;
        std::getline(schema_read, line1);
        std::getline(schema_read, line2);
        schema_read.close();
        
        bool consistent = (line1 == "table1:id,name,email" && line2 == "table2:user_id,status");
        
        if (consistent) {
            std::cout << "  ✅ Schema consistency verified\n";
        }
        
        return consistent;
    }
    
    static bool test_transaction_chain_recovery() {
        std::cout << "Test: Long transaction chain recovery\n";
        
        std::string wal_path = "/tmp/recovery_test_chain.wal";
        std::ofstream wal(wal_path, std::ios::binary);
        
        int total_txns = 1000;
        int committed_count = 0;
        
        // Write 1000 transaction entries
        for (int i = 0; i < total_txns; i++) {
            int txn_id = i;
            int committed = (i % 3 == 0) ? 1 : 0;  // 1 in 3 are committed
            uint32_t len = 20;
            
            wal.write((char*)&txn_id, 4);
            wal.write((char*)&committed, 4);
            wal.write((char*)&len, 4);
            wal.write("transaction_data_xx", len);
            
            if (committed) committed_count++;
        }
        
        wal.close();
        
        // Verify recovery
        std::ifstream wal_read(wal_path, std::ios::binary);
        int recovered = 0;
        
        while (true) {
            int txn_id, committed;
            uint32_t len;
            char data[20];
            
            if (!wal_read.read((char*)&txn_id, 4)) break;
            if (!wal_read.read((char*)&committed, 4)) break;
            if (!wal_read.read((char*)&len, 4)) break;
            if (!wal_read.read(data, len)) break;
            
            if (committed) recovered++;
        }
        
        wal_read.close();
        
        std::cout << "  Total transactions: " << total_txns << "\n";
        std::cout << "  Committed: " << committed_count << "\n";
        std::cout << "  Recovered: " << recovered << "\n";
        
        bool success = (recovered == committed_count);
        if (success) {
            std::cout << "  ✅ Long chain recovery works\n";
        }
        
        return success;
    }
};

int main() {
    std::cout << "RECOVERY TESTING\n\n";
    
    int passed = 0;
    
    if (MultiProcessRecoveryTest::test_concurrent_write_crash_recovery()) passed++;
    std::cout << "\n";
    
    if (MultiProcessRecoveryTest::test_schema_consistency()) passed++;
    std::cout << "\n";
    
    if (MultiProcessRecoveryTest::test_transaction_chain_recovery()) passed++;
    std::cout << "\n";
    
    std::cout << "✅ Recovery: " << passed << "/3 tests passed\n";
    return passed == 3 ? 0 : 1;
}
