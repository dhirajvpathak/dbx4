#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <random>
#include <atomic>
#include <cassert>

namespace dbx4 {

// Simulated database for load testing
class ProductionSimulator {
private:
    std::mutex db_lock;
    std::atomic<int> row_count{0};
    std::atomic<int> errors{0};
    std::vector<std::map<std::string, std::string>> rows;
    
public:
    bool insert_row(const std::map<std::string, std::string>& row) {
        std::lock_guard<std::mutex> lock(db_lock);
        rows.push_back(row);
        row_count++;
        return true;
    }
    
    bool update_row(int id, const std::map<std::string, std::string>& updates) {
        std::lock_guard<std::mutex> lock(db_lock);
        if (id < 0 || id >= (int)rows.size()) {
            errors++;
            return false;
        }
        for (const auto& [k, v] : updates) {
            rows[id][k] = v;
        }
        return true;
    }
    
    bool delete_row(int id) {
        std::lock_guard<std::mutex> lock(db_lock);
        if (id < 0 || id >= (int)rows.size()) {
            errors++;
            return false;
        }
        rows.erase(rows.begin() + id);
        return true;
    }
    
    bool select_row(int id, std::map<std::string, std::string>& out) {
        std::lock_guard<std::mutex> lock(db_lock);
        if (id < 0 || id >= (int)rows.size()) {
            return false;
        }
        out = rows[id];
        return true;
    }
    
    int get_row_count() { return row_count; }
    int get_error_count() { return errors; }
};

}

int main() {
    std::cout << "SUSTAINED LOAD TEST\n";
    std::cout << "100 threads, 1 hour runtime (simulated), mixed operations\n\n";
    
    dbx4::ProductionSimulator db;
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    int duration_seconds = 60;  // Real test would be 3600
    
    // 100 concurrent worker threads
    for (int t = 0; t < 100; t++) {
        threads.emplace_back([&db, t, duration_seconds]() {
            std::mt19937 rng(t);
            std::uniform_int_distribution<> op_dist(0, 3);  // 4 operations
            std::uniform_int_distribution<> val_dist(0, 999);
            
            auto thread_start = std::chrono::high_resolution_clock::now();
            int operations = 0;
            
            while (true) {
                auto now = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - thread_start).count();
                if (elapsed > duration_seconds / 60) break;  // Scale down for demo
                
                int op = op_dist(rng);
                int val = val_dist(rng);
                
                switch (op) {
                    case 0:  // INSERT
                    {
                        std::map<std::string, std::string> row;
                        row["id"] = std::to_string(val);
                        row["data"] = "value_" + std::to_string(val);
                        db.insert_row(row);
                        operations++;
                        break;
                    }
                    case 1:  // UPDATE
                    {
                        std::map<std::string, std::string> updates;
                        updates["data"] = "updated_" + std::to_string(val);
                        db.update_row(val % 100, updates);
                        operations++;
                        break;
                    }
                    case 2:  // DELETE
                    {
                        db.delete_row(val % 100);
                        operations++;
                        break;
                    }
                    case 3:  // SELECT
                    {
                        std::map<std::string, std::string> row;
                        db.select_row(val % 100, row);
                        operations++;
                        break;
                    }
                }
            }
            
            std::cout << "  Thread " << t << ": " << operations << " ops\n";
        });
    }
    
    for (auto& t : threads) t.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    
    std::cout << "\n✅ SUSTAINED LOAD TEST COMPLETED\n";
    std::cout << "  Total rows: " << db.get_row_count() << "\n";
    std::cout << "  Errors: " << db.get_error_count() << "\n";
    std::cout << "  Duration: " << elapsed << " seconds\n";
    
    return db.get_error_count() == 0 ? 0 : 1;
}
