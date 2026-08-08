#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <cassert>

int main() {
    std::cout << "Concurrency Test\n";
    std::mutex lock;
    int counter = 0;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 1000; j++) {
                std::lock_guard<std::mutex> lck(lock);
                counter++;
            }
        });
    }
    
    for (auto& t : threads) t.join();
    
    assert(counter == 10000);
    std::cout << "✅ PASS: 10 threads, 10000 operations, no race\n";
    return 0;
}
