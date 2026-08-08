#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <atomic>
#include <random>
#include <cassert>

struct LoadTestMetrics {
    std::atomic<int> total_ops{0};
    std::atomic<int> insert_ops{0};
    std::atomic<int> update_ops{0};
    std::atomic<int> delete_ops{0};
    std::atomic<int> select_ops{0};
    std::atomic<int> errors{0};
};

class ConcurrentLoadTester {
private:
    LoadTestMetrics metrics;
    std::mutex db_lock;
    int record_count = 0;
    
public:
    void simulate_insert() {
        std::lock_guard<std::mutex> lock(db_lock);
        record_count++;
        metrics.insert_ops++;
        metrics.total_ops++;
    }
    
    void simulate_update() {
        std::lock_guard<std::mutex> lock(db_lock);
        if (record_count > 0) {
            metrics.update_ops++;
            metrics.total_ops++;
        }
    }
    
    void simulate_delete() {
        std::lock_guard<std::mutex> lock(db_lock);
        if (record_count > 0) {
            record_count--;
            metrics.delete_ops++;
            metrics.total_ops++;
        }
    }
    
    void simulate_select() {
        std::lock_guard<std::mutex> lock(db_lock);
        int local_count = record_count;
        metrics.select_ops++;
        metrics.total_ops++;
    }
    
    void worker_thread(int thread_id, int ops_per_thread) {
        std::random_device rd;
        std::mt19937 gen(rd() + thread_id);
        std::uniform_int_distribution<> dis(0, 3);  // 0=INSERT, 1=UPDATE, 2=DELETE, 3=SELECT
        
        for (int i = 0; i < ops_per_thread; i++) {
            int op = dis(gen);
            try {
                switch (op) {
                    case 0: simulate_insert(); break;
                    case 1: simulate_update(); break;
                    case 2: simulate_delete(); break;
                    case 3: simulate_select(); break;
                }
            } catch (...) {
                metrics.errors++;
            }
        }
    }
    
    bool run_test(int num_threads, int ops_per_thread) {
        std::cout << "Running concurrent load test: " << num_threads << " threads, " 
                  << ops_per_thread << " ops/thread\n";
        
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
        std::cout << "  INSERT: " << metrics.insert_ops << "\n";
        std::cout << "  UPDATE: " << metrics.update_ops << "\n";
        std::cout << "  DELETE: " << metrics.delete_ops << "\n";
        std::cout << "  SELECT: " << metrics.select_ops << "\n";
        std::cout << "  Errors: " << metrics.errors << "\n";
        std::cout << "  Throughput: " << (int)throughput << " ops/sec\n";
        std::cout << "  Duration: " << duration_ms.count() << "ms\n";
        
        return metrics.errors.load() == 0 && throughput >= 1000;
    }
};

int main() {
    std::cout << "CONCURRENT LOAD TESTING\n\n";
    
    ConcurrentLoadTester tester;
    
    // Test 1: 50 threads, 1000 ops each
    bool test1 = tester.run_test(50, 1000);
    std::cout << (test1 ? "✅" : "❌") << " Test 1 (50 threads): " << (test1 ? "PASS" : "FAIL") << "\n\n";
    
    // Test 2: 100 threads, 500 ops each
    ConcurrentLoadTester tester2;
    bool test2 = tester2.run_test(100, 500);
    std::cout << (test2 ? "✅" : "❌") << " Test 2 (100 threads): " << (test2 ? "PASS" : "FAIL") << "\n\n";
    
    if (test1 && test2) {
        std::cout << "✅ Concurrency: All tests passed\n";
        return 0;
    } else {
        std::cout << "❌ Concurrency: Some tests failed\n";
        return 1;
    }
}
