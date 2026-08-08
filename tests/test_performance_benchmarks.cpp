#include <iostream>
#include <chrono>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <mutex>

struct BenchmarkResult {
    std::string name;
    int64_t total_ops;
    int64_t duration_ms;
    double throughput;
    double latency_ms;
};

class PerformanceBenchmark {
private:
    std::mutex lock;
    int data_store = 0;
    
public:
    BenchmarkResult benchmark_inserts(int num_ops) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_ops; i++) {
            std::lock_guard<std::mutex> lck(lock);
            data_store = i;  // Simulate insert
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        BenchmarkResult result;
        result.name = "INSERT";
        result.total_ops = num_ops;
        result.duration_ms = duration.count();
        result.throughput = (num_ops * 1000.0) / duration.count();
        result.latency_ms = duration.count() / (double)num_ops;
        
        return result;
    }
    
    BenchmarkResult benchmark_selects(int num_ops) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_ops; i++) {
            std::lock_guard<std::mutex> lck(lock);
            volatile int val = data_store;
            (void)val;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        BenchmarkResult result;
        result.name = "SELECT";
        result.total_ops = num_ops;
        result.duration_ms = duration.count();
        result.throughput = (num_ops * 1000.0) / duration.count();
        result.latency_ms = duration.count() / (double)num_ops;
        
        return result;
    }
    
    BenchmarkResult benchmark_updates(int num_ops) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_ops; i++) {
            std::lock_guard<std::mutex> lck(lock);
            data_store = data_store + 1;  // Simulate update
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        BenchmarkResult result;
        result.name = "UPDATE";
        result.total_ops = num_ops;
        result.duration_ms = duration.count();
        result.throughput = (num_ops * 1000.0) / duration.count();
        result.latency_ms = duration.count() / (double)num_ops;
        
        return result;
    }
    
    void print_result(const BenchmarkResult& r) {
        std::cout << std::left << std::setw(12) << r.name
                  << std::setw(15) << r.total_ops
                  << std::setw(12) << r.duration_ms
                  << std::fixed << std::setprecision(2)
                  << std::setw(15) << r.throughput
                  << std::setw(12) << r.latency_ms
                  << " µs\n";
    }
};

int main() {
    std::cout << "PERFORMANCE BENCHMARKING\n\n";
    
    PerformanceBenchmark bench;
    std::vector<BenchmarkResult> results;
    
    // Warmup
    bench.benchmark_inserts(1000);
    
    // Benchmarks
    std::cout << "Operation    Operations    Duration(ms)  Throughput(ops/sec)  Latency\n";
    std::cout << "========================================================================\n";
    
    auto insert_result = bench.benchmark_inserts(100000);
    bench.print_result(insert_result);
    results.push_back(insert_result);
    
    auto select_result = bench.benchmark_selects(100000);
    bench.print_result(select_result);
    results.push_back(select_result);
    
    auto update_result = bench.benchmark_updates(100000);
    bench.print_result(update_result);
    results.push_back(update_result);
    
    std::cout << "\nPERFORMANCE SUMMARY\n";
    std::cout << "====================\n";
    
    double avg_throughput = 0;
    for (const auto& r : results) {
        avg_throughput += r.throughput;
    }
    avg_throughput /= results.size();
    
    std::cout << "Average Throughput: " << (int)avg_throughput << " ops/sec\n";
    std::cout << "Status: " << (avg_throughput >= 10000 ? "✅ PASS (>= 10k ops/sec)" : "❌ FAIL") << "\n";
    
    return avg_throughput >= 10000 ? 0 : 1;
}
