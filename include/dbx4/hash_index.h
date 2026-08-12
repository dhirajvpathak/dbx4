#ifndef DBX4_HASH_INDEX_H
#define DBX4_HASH_INDEX_H

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>

namespace dbx4 {

class HashIndex {
private:
    std::string index_name_;
    std::string table_name_;
    std::string column_;
    std::unordered_map<std::string, std::vector<int>> hash_map_;
    
public:
    HashIndex(const std::string& name, const std::string& table, const std::string& col)
        : index_name_(name), table_name_(table), column_(col) {}
    
    // Insert key -> row_id mapping
    bool insert(const std::string& key, int row_id) {
        hash_map_[key].push_back(row_id);
        return true;
    }
    
    // O(1) lookup - perfect for equality queries
    std::vector<int> lookup(const std::string& key) {
        auto it = hash_map_.find(key);
        if (it != hash_map_.end()) {
            return it->second;
        }
        return {};
    }
    
    // Delete entry
    bool delete_entry(const std::string& key, int row_id) {
        auto it = hash_map_.find(key);
        if (it != hash_map_.end()) {
            auto& rows = it->second;
            auto row_it = std::find(rows.begin(), rows.end(), row_id);
            if (row_it != rows.end()) {
                rows.erase(row_it);
                if (rows.empty()) {
                    hash_map_.erase(it);
                }
                return true;
            }
        }
        return false;
    }
    
    // Get statistics
    size_t get_bucket_count() const {
        return hash_map_.bucket_count();
    }
    
    size_t get_size() const {
        return hash_map_.size();
    }
    
    double get_load_factor() const {
        return hash_map_.load_factor();
    }
    
    // Check if key exists
    bool contains(const std::string& key) const {
        return hash_map_.find(key) != hash_map_.end();
    }
    
    std::string get_index_name() const { return index_name_; }
    std::string get_table_name() const { return table_name_; }
    std::string get_column() const { return column_; }
    
    void print_stats() {
        std::cout << "[HashIndex] " << index_name_ << " on " << table_name_ 
                  << "(" << column_ << ")\n";
        std::cout << "  Keys: " << hash_map_.size() << "\n";
        std::cout << "  Load factor: " << hash_map_.load_factor() << "\n";
        std::cout << "  Bucket count: " << hash_map_.bucket_count() << "\n";
    }
};

}

#endif
