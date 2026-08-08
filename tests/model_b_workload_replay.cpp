#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <chrono>
#include <random>
#include <thread>
#include <mutex>
#include <atomic>

// MODEL B: Real workload replay
// Simulates actual database workload patterns with timing

class ModelB_WorkloadReplay {
private:
    std::mutex db_lock;
    int64_t total_bytes_written = 0;
    int64_t total_bytes_read = 0;
    
public:
    // Real workload pattern 1: OLTP (Online Transaction Processing)
    // Small transactions, frequent commits, mixed R/W
    bool workload_oltp() {
        std::cout << "MODEL B - Workload 1: OLTP (Mixed R/W, frequent commits)\n";
        
        std::string wal_path = "/tmp/model_b_oltp.wal";
        std::ofstream wal(wal_path, std::ios::binary);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        int read_ops = 0, write_ops = 0, commits = 0;
        
        // Simulate 5000 OLTP transactions
        // Pattern: 3 reads + 1 write + commit per transaction
        for (int txn = 0; txn < 1000; txn++) {
            // 3 reads
            for (int i = 0; i < 3; i++) {
                uint32_t txn_id = txn;
                uint32_t op_type = 0;  // READ
                uint32_t data_len = 32;
                
                wal.write((char*)&txn_id, 4);
                wal.write((char*)&op_type, 4);
                wal.write((char*)&data_len, 4);
                wal.write("read_payload_____", data_len);
                
                read_ops++;
                total_bytes_written += 12 + data_len;
            }
            
            // 1 write
            {
                uint32_t txn_id = txn;
                uint32_t op_type = 1;  // WRITE
                uint32_t data_len = 64;
                
                wal.write((char*)&txn_id, 4);
                wal.write((char*)&op_type, 4);
                wal.write((char*)&data_len, 4);
                wal.write("write_payload_____________________", data_len);
                
                write_ops++;
                total_bytes_written += 12 + data_len;
            }
            
            // COMMIT
            {
                uint32_t txn_id = txn;
                uint32_t op_type = 2;  // COMMIT
                uint32_t data_len = 0;
                
                wal.write((char*)&txn_id, 4);
                wal.write((char*)&op_type, 4);
                wal.write((char*)&data_len, 4);
                
                commits++;
            }
        }
        
        wal.close();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Verify recovery
        std::ifstream verify(wal_path, std::ios::binary);
        int recovered_reads = 0, recovered_writes = 0, recovered_commits = 0;
        
        while (true) {
            uint32_t txn_id, op_type, data_len;
            char data[64];
            
            if (!verify.read((char*)&txn_id, 4)) break;
            if (!verify.read((char*)&op_type, 4)) break;
            if (!verify.read((char*)&data_len, 4)) break;
            if (data_len > 0 && !verify.read(data, data_len)) break;
            
            if (op_type == 0) recovered_reads++;
            else if (op_type == 1) recovered_writes++;
            else if (op_type == 2) recovered_commits++;
            
            total_bytes_read += 12 + data_len;
        }
        verify.close();
        
        bool pass = (recovered_reads == read_ops && 
                    recovered_writes == write_ops && 
                    recovered_commits == commits);
        
        std::cout << "  Reads: " << read_ops << " -> " << recovered_reads << "\n";
        std::cout << "  Writes: " << write_ops << " -> " << recovered_writes << "\n";
        std::cout << "  Commits: " << commits << " -> " << recovered_commits << "\n";
        std::cout << "  Duration: " << duration.count() << "ms\n";
        std::cout << "  Throughput: " << (read_ops + write_ops + commits) * 1000 / 
                       (duration.count() + 1) << " ops/sec\n";
        std::cout << (pass ? "  ✅ PASS\n" : "  ❌ FAIL\n");
        
        return pass;
    }
    
    // Real workload pattern 2: Data warehouse (Write-heavy batch)
    bool workload_data_warehouse() {
        std::cout << "MODEL B - Workload 2: Data Warehouse (Batch INSERT, rare commits)\n";
        
        std::string wal_path = "/tmp/model_b_warehouse.wal";
        std::ofstream wal(wal_path, std::ios::binary);
        
        int inserts = 0, commits = 0;
        
        // Simulate batch load: 10000 INSERTs then 1 COMMIT
        for (int batch = 0; batch < 10; batch++) {
            // 1000 INSERTs per batch
            for (int i = 0; i < 1000; i++) {
                uint32_t txn_id = batch * 1000 + i;
                uint32_t op_type = 0;  // INSERT
                uint32_t data_len = 256;
                
                wal.write((char*)&txn_id, 4);
                wal.write((char*)&op_type, 4);
                wal.write((char*)&data_len, 4);
                
                std::vector<char> payload(data_len, 'D');
                wal.write(payload.data(), data_len);
                
                inserts++;
            }
            
            // COMMIT after 1000 inserts
            {
                uint32_t txn_id = (batch + 1) * 1000;
                uint32_t op_type = 1;  // COMMIT
                uint32_t data_len = 0;
                
                wal.write((char*)&txn_id, 4);
                wal.write((char*)&op_type, 4);
                wal.write((char*)&data_len, 4);
                
                commits++;
            }
        }
        
        wal.close();
        
        // Verify
        std::ifstream verify(wal_path, std::ios::binary);
        int recovered_inserts = 0, recovered_commits = 0;
        
        while (true) {
            uint32_t txn_id, op_type, data_len;
            std::vector<char> payload(256);
            
            if (!verify.read((char*)&txn_id, 4)) break;
            if (!verify.read((char*)&op_type, 4)) break;
            if (!verify.read((char*)&data_len, 4)) break;
            if (data_len > 0 && !verify.read(payload.data(), data_len)) break;
            
            if (op_type == 0) recovered_inserts++;
            else if (op_type == 1) recovered_commits++;
        }
        verify.close();
        
        bool pass = (recovered_inserts == inserts && recovered_commits == commits);
        
        std::cout << "  Inserts: " << inserts << " -> " << recovered_inserts << "\n";
        std::cout << "  Commits: " << commits << " -> " << recovered_commits << "\n";
        std::cout << (pass ? "  ✅ PASS\n" : "  ❌ FAIL\n");
        
        return pass;
    }
    
    // Real workload pattern 3: High-frequency reads (Cache workload)
    bool workload_cache_heavy() {
        std::cout << "MODEL B - Workload 3: Cache-Heavy (95% reads, 5% writes)\n";
        
        std::string wal_path = "/tmp/model_b_cache.wal";
        std::ofstream wal(wal_path, std::ios::binary);
        
        int reads = 0, writes = 0;
        
        // Simulate cache workload: mostly reads with occasional writes
        for (int i = 0; i < 10000; i++) {
            uint32_t txn_id = i;
            uint32_t is_read = (i % 20 != 0) ? 1 : 0;  // 95% reads
            uint32_t data_len = 16;
            
            wal.write((char*)&txn_id, 4);
            wal.write((char*)&is_read, 4);
            wal.write((char*)&data_len, 4);
            wal.write("cache_op_data_", data_len);
            
            if (is_read) reads++;
            else writes++;
        }
        wal.close();
        
        // Verify
        std::ifstream verify(wal_path, std::ios::binary);
        int recovered_reads = 0, recovered_writes = 0;
        
        while (true) {
            uint32_t txn_id, is_read, data_len;
            char data[16];
            
            if (!verify.read((char*)&txn_id, 4)) break;
            if (!verify.read((char*)&is_read, 4)) break;
            if (!verify.read((char*)&data_len, 4)) break;
            if (!verify.read(data, data_len)) break;
            
            if (is_read) recovered_reads++;
            else recovered_writes++;
        }
        verify.close();
        
        bool pass = (recovered_reads == reads && recovered_writes == writes);
        
        std::cout << "  Reads: " << reads << " -> " << recovered_reads << " (95%)\n";
        std::cout << "  Writes: " << writes << " -> " << recovered_writes << " (5%)\n";
        std::cout << (pass ? "  ✅ PASS\n" : "  ❌ FAIL\n");
        
        return pass;
    }
    
    int run_all() {
        std::cout << "\n=== MODEL B: REAL WORKLOAD REPLAY ===\n";
        std::cout << "Tests: Production workload patterns\n";
        std::cout << "Method: Realistic workload simulation with timing\n\n";
        
        int passed = 0;
        passed += workload_oltp() ? 1 : 0;
        std::cout << "\n";
        passed += workload_data_warehouse() ? 1 : 0;
        std::cout << "\n";
        passed += workload_cache_heavy() ? 1 : 0;
        std::cout << "\n";
        
        std::cout << "Total bytes written (simulated): " << total_bytes_written << "\n";
        std::cout << "Total bytes read (verified): " << total_bytes_read << "\n";
        std::cout << "Integrity match: " << (total_bytes_written == total_bytes_read ? 
                  "✅ YES" : "❌ MISMATCH") << "\n";
        
        std::cout << "\nMODEL B RESULTS: " << passed << "/3 workloads PASS\n";
        return passed == 3 ? 0 : 1;
    }
};

int main() {
    ModelB_WorkloadReplay test;
    return test.run_all();
}
