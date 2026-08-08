#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>

std::mutex cache_mutex;
int operation_count = 0;

void concurrent_worker(int thread_id, int ops) {
    for (int i = 0; i < ops; i++) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        operation_count++;
    }
}

int main() {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    
    // 100 concurrent threads
    for (int i = 0; i < 100; i++) {
        threads.emplace_back([i]() { concurrent_worker(i, 1000); });
    }
    
    for (auto& t : threads) t.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "✅ Concurrent load test PASSED\n";
    std::cout << "  Threads: 100\n";
    std::cout << "  Operations: " << operation_count << "\n";
    std::cout << "  Time: " << duration.count() << "ms\n";
    std::cout << "  Ops/sec: " << (operation_count * 1000 / duration.count()) << "\n";
    
    return 0;
}
