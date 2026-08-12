#ifndef DBX4_BTREE_INDEX_H
#define DBX4_BTREE_INDEX_H

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace dbx4 {

struct IndexKey {
    std::vector<std::string> columns;
    std::string key_value;
};

struct IndexEntry {
    IndexKey key;
    std::vector<int> row_ids;  // Rows matching this key
};

class BTreeIndex {
private:
    std::string index_name_;
    std::string table_name_;
    std::vector<std::string> columns_;
    std::map<std::string, std::vector<int>> index_map_;
    bool is_partial_ = false;
    std::string partial_where_;
    
public:
    BTreeIndex(const std::string& name, const std::string& table)
        : index_name_(name), table_name_(table) {}
    
    void add_column(const std::string& col) {
        columns_.push_back(col);
    }
    
    void set_partial(const std::string& where_clause) {
        is_partial_ = true;
        partial_where_ = where_clause;
    }
    
    bool insert(const std::string& key, int row_id) {
        index_map_[key].push_back(row_id);
        return true;
    }
    
    std::vector<int> lookup(const std::string& key) {
        if (index_map_.find(key) != index_map_.end()) {
            return index_map_[key];
        }
        return {};
    }
    
    bool delete_entry(const std::string& key, int row_id) {
        if (index_map_.find(key) == index_map_.end()) return false;
        
        auto& rows = index_map_[key];
        auto it = std::find(rows.begin(), rows.end(), row_id);
        if (it != rows.end()) {
            rows.erase(it);
            return true;
        }
        return false;
    }
    
    int get_estimated_rows(const std::string& key) {
        if (index_map_.find(key) != index_map_.end()) {
            return index_map_[key].size();
        }
        return 0;
    }
    
    std::string get_index_name() const { return index_name_; }
    std::string get_table_name() const { return table_name_; }
    std::vector<std::string> get_columns() const { return columns_; }
    bool is_partial() const { return is_partial_; }
    
    void print_stats() {
        std::cout << "[Index] " << index_name_ << " on " << table_name_;
        for (const auto& col : columns_) {
            std::cout << "(" << col << ")";
        }
        std::cout << " - " << index_map_.size() << " keys\n";
    }
};

}

#endif
