#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <cstring>
#include <iomanip>

class ConcurrencyTestSuite {
private:
    std::mutex db_lock;
    std::vector<std::string> data_rows;
    
    struct Metrics {
        std::atomic<int64_t> total_ops{0};
        std::atomic<int64_t> insert_ops{0};
        std::atomic<int64_t> select_ops{0};
        std::atomic<int64_t> update_ops{0};
        std::atomic<int64_t> delete_ops{0};
        std::atomic<int64_t> errors{0};
        std::atomic<int64_t> total_latency_us{0};
        std::atomic<int64_t> max_latency_us{0};
    };
    
    Metrics metrics;
    
public:
    void worker_thread(int thread_id, int ops_per_thread) {
        std::random_device rd;
        std::mt19937 gen(rd() + thread_id);
        std::uniform_int_distribution<> op_dist(0, 3);
        
        for (int i = 0; i < ops_per_thread; i++) {
            int op = op_dist(gen);
            
            auto start = std::chrono::high_resolution_clock::now();
            
            try {
                std::lock_guard<std::mutex> lock(db_lock);
                
                switch (op) {
                    case 0:
                        data_rows.push_back("row_" + std::to_string(thread_id) + "_" + std::to_string(i));
                        metrics.insert_ops++;
                        break;
                    case 1:
                        if (!data_rows.empty()) {
                            volatile auto val = data_rows.back();
                            (void)val;
                        }
                        metrics.select_ops++;
                        break;
                    case 2:
                        if (!data_rows.empty()) {
                            data_rows[data_rows.size() / 2] = "updated_" + std::to_string(i);
                        }
                        metrics.update_ops++;
                        break;
                    case 3:
                        if (!data_rows.empty()) {
                            data_rows.pop_back();
                        }
                        metrics.delete_ops++;
                        break;
                }
                metrics.total_ops++;
                
            } catch (...) {
                metrics.errors++;
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            metrics.total_latency_us += latency.count();
            auto current_max = metrics.max_latency_us.load();
            while (latency.count() > current_max && 
                   !metrics.max_latency_us.compare_exchange_weak(current_max, latency.count())) {
                current_max = metrics.max_latency_us.load();
            }
        }
    }
    
    bool test_50_concurrent_clients() {
        std::cout << "TEST 1: 50 concurrent clients (50k ops)\n";
        
        data_rows.clear();
        metrics = Metrics();
        
        int num_threads = 50;
        int ops_per_thread = 1000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back([this, i, ops_per_thread]() {
                worker_thread(i, ops_per_thread);
            });
        }
        
        for (auto& t : threads) t.join();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        double throughput = (metrics.total_ops.load() * 1000.0) / duration_ms.count();
        double avg_latency = metrics.total_ops.load() > 0 ? 
            metrics.total_latency_us.load() / (double)metrics.total_ops.load() : 0;
        
        std::cout << "  Total ops: " << metrics.total_ops << "\n";
        std::cout << "  Errors: " << metrics.errors << "\n";
        std::cout << "  Duration: " << duration_ms.count() << "ms\n";
        std::cout << "  Throughput: " << (int)throughput << " ops/sec\n";
        std::cout << "  Avg latency: " << std::fixed << std::setprecision(1) << avg_latency << " µs\n";
        std::cout << "  Max latency: " << metrics.max_latency_us << " µs\n";
        
        bool pass = (metrics.errors.load() == 0 && throughput >= 10000);
        std::cout << (pass ? "  ✅ PASS" : "  ❌ FAIL") << "\n";
        return pass;
    }
    
    bool test_100_concurrent_clients() {
        std::cout << "TEST 2: 100 concurrent clients (100k ops)\n";
        
        data_rows.clear();
        metrics = Metrics();
        
        int num_threads = 100;
        int ops_per_thread = 1000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back([this, i, ops_per_thread]() {
                worker_thread(i, ops_per_thread);
            });
        }
        
        for (auto& t : threads) t.join();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        double throughput = (metrics.total_ops.load() * 1000.0) / duration_ms.count();
        
        std::cout << "  Total ops: " << metrics.total_ops << "\n";
        std::cout << "  Errors: " << metrics.errors << "\n";
        std::cout << "  Duration: " << duration_ms.count() << "ms\n";
        std::cout << "  Throughput: " << (int)throughput << " ops/sec\n";
        
        bool pass = (metrics.errors.load() == 0);
        std::cout << (pass ? "  ✅ PASS" : "  ❌ FAIL") << "\n";
        return pass;
    }
    
    int run_all() {
        std::cout << "================================================\n";
        std::cout << "CONCURRENCY TEST SUITE\n";
        std::cout << "================================================\n\n";
        
        int passed = 0;
        passed += (test_50_concurrent_clients() ? 1 : 0);
        std::cout << "\n";
        passed += (test_100_concurrent_clients() ? 1 : 0);
        std::cout << "\n";
        
        std::cout << "================================================\n";
        std::cout << "CONCURRENCY: " << passed << "/2 PASS\n";
        std::cout << "================================================\n";
        
        return passed == 2 ? 0 : 1;
    }
};

int main() {
    ConcurrencyTestSuite suite;
    return suite.run_all();
}
