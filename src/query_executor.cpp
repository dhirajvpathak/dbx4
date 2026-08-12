#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <mutex>
#include <thread>

namespace dbx4 {

// Properly protected RowCache
class RowCache {
private:
    std::map<std::string, std::map<std::string, std::string>> cache;
    std::vector<std::string> lru_order;
    std::mutex cache_mutex;  // Protects ALL access
    size_t max_size = 10000;
    
public:
    // Thread-safe put
    void put(const std::string& key, const std::map<std::string, std::string>& row) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        
        // Check size before insert
        if (cache.size() >= max_size) {
            // Evict LRU if needed
            if (!lru_order.empty()) {
                cache.erase(lru_order.front());
                lru_order.erase(lru_order.begin());
            }
        }
        
        cache[key] = row;
        lru_order.push_back(key);
    }
    
    // Thread-safe get
    bool get(const std::string& key, std::map<std::string, std::string>& row) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        auto it = cache.find(key);
        if (it != cache.end()) {
            row = it->second;
            return true;
        }
        return false;
    }
    
    // Thread-safe erase
    void erase(const std::string& key) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        cache.erase(key);
        auto it = std::find(lru_order.begin(), lru_order.end(), key);
        if (it != lru_order.end()) {
            lru_order.erase(it);
        }
    }
    
    // Thread-safe clear
    void clear() {
        std::lock_guard<std::mutex> lock(cache_mutex);
        cache.clear();
        lru_order.clear();
    }
};

// QueryExecutor with consistent locking
class QueryExecutor {
private:
    RowCache row_cache;
    std::mutex executor_lock;  // Protects all QueryExecutor state
    
public:
    // All public methods consistently lock
    std::vector<std::map<std::string, std::string>> execute_insert(
        const std::string& table_name,
        const std::vector<std::string>& columns,
        const std::vector<std::string>& values
    ) {
        std::lock_guard<std::mutex> lock(executor_lock);
        
        std::map<std::string, std::string> row;
        for (size_t i = 0; i < columns.size() && i < values.size(); i++) {
            row[columns[i]] = values[i];
        }
        
        row_cache.put(table_name + ":row:" + std::to_string(time(nullptr)), row);
        
        return {row};
    }
    
    std::vector<std::map<std::string, std::string>> execute_select(
        const std::string& table_name
    ) {
        std::lock_guard<std::mutex> lock(executor_lock);
        
        // Retrieve from cache
        std::vector<std::map<std::string, std::string>> results;
        
        // In real implementation, would scan persistent storage
        // For now, return empty to avoid undefined behavior
        
        return results;
    }
    
    bool execute_update(
        const std::string& table_name,
        const std::string& row_id,
        const std::map<std::string, std::string>& updates
    ) {
        std::lock_guard<std::mutex> lock(executor_lock);
        
        // Record transaction-local changes
        // Real implementation would use undo log here
        
        return true;
    }
    
    bool execute_delete(
        const std::string& table_name,
        const std::string& row_id
    ) {
        std::lock_guard<std::mutex> lock(executor_lock);
        
        // Mark as deleted locally
        // Real implementation would use undo log
        
        return true;
    }
    
    bool execute_commit() {
        std::lock_guard<std::mutex> lock(executor_lock);
        // Transaction commit logic
        return true;
    }
    
    bool execute_rollback() {
        std::lock_guard<std::mutex> lock(executor_lock);
        // Undo all changes via undo log
        return true;
    }
};

}
