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

namespace dbx4 {

struct RowVersion {
    int version_id = 0;
    bool deleted = false;
    std::map<std::string, std::string> data;
    
    bool is_deleted() const { return deleted; }
};

class RowCache {
private:
    std::map<std::string, std::map<std::string, std::string>> cache;
    std::vector<std::string> lru_order;
    std::mutex cache_mutex;
    size_t max_size = 1000;
    size_t current_size = 0;
    
public:
    void put(const std::string& key, const std::map<std::string, std::string>& row) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        cache[key] = row;
        lru_order.push_back(key);
        current_size++;
    }
    
    bool get(const std::string& key, std::map<std::string, std::string>& row) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        auto it = cache.find(key);
        if (it != cache.end()) {
            row = it->second;
            return true;
        }
        return false;
    }
};

class QueryExecutor {
private:
    std::map<std::string, std::vector<RowVersion>> table_versions;
    RowCache row_cache;
    std::mutex executor_lock;
    
public:
    // INSERT
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
        
        RowVersion version;
        version.version_id = 1;
        version.deleted = false;
        version.data = row;
        
        table_versions[table_name].push_back(version);
        row_cache.put(table_name + "_" + std::to_string(version.version_id), row);
        
        return {row};
    }
    
    // SELECT
    std::vector<std::map<std::string, std::string>> execute_select(
        const std::string& table_name
    ) {
        std::lock_guard<std::mutex> lock(executor_lock);
        
        std::vector<std::map<std::string, std::string>> result;
        
        if (table_versions.find(table_name) == table_versions.end()) {
            return result;
        }
        
        for (const auto& version : table_versions[table_name]) {
            if (!version.is_deleted()) {
                result.push_back(version.data);
            }
        }
        
        return result;
    }
    
    // UPDATE - FIXED: declare current_version properly
    std::vector<std::map<std::string, std::string>> execute_update(
        const std::string& table_name,
        const std::map<std::string, std::string>& new_values
    ) {
        std::lock_guard<std::mutex> lock(executor_lock);
        
        if (table_versions.find(table_name) == table_versions.end()) {
            return {};
        }
        
        auto& versions = table_versions[table_name];
        if (versions.empty()) {
            return {};
        }
        
        // Get current version and create new version
        auto& current_version = versions.back();
        
        RowVersion new_version;
        new_version.version_id = current_version.version_id + 1;
        new_version.deleted = false;
        new_version.data = current_version.data;
        
        // Apply updates
        for (const auto& kv : new_values) {
            new_version.data[kv.first] = kv.second;
        }
        
        versions.push_back(new_version);
        
        return {new_version.data};
    }
    
    // DELETE - FIXED: declare current_version properly
    std::vector<std::map<std::string, std::string>> execute_delete(
        const std::string& table_name
    ) {
        std::lock_guard<std::mutex> lock(executor_lock);
        
        if (table_versions.find(table_name) == table_versions.end()) {
            return {};
        }
        
        auto& versions = table_versions[table_name];
        
        std::vector<std::map<std::string, std::string>> result;
        
        for (auto& version : versions) {
            if (version.is_deleted()) continue;
            
            // Get current version before deleting
            auto& current_version = version;
            
            result.push_back(current_version.data);
            
            // Create delete marker
            RowVersion delete_version;
            delete_version.version_id = current_version.version_id + 1;
            delete_version.deleted = true;
            delete_version.data = current_version.data;
            
            versions.push_back(delete_version);
        }
        
        return result;
    }
};

}
