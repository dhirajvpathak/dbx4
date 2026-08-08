#ifndef DBX4_UNIFIED_ROW_H
#define DBX4_UNIFIED_ROW_H

#include <string>
#include <map>
#include <vector>
#include <cstdint>

namespace dbx4 {

// UNIFIED ROW DEFINITION - SINGLE SOURCE OF TRUTH
// Used across ALL storage, SQL, and executor code
struct Row {
    // Column data: column_name -> column_value
    std::map<std::string, std::string> columns;
    
    // Row metadata
    uint64_t row_id = 0;
    bool is_deleted = false;
    uint64_t version = 0;
    
    // Binary representation for storage
    std::vector<uint8_t> serialized_data;
    
    // Convenience methods
    std::string get(const std::string& column_name) const {
        auto it = columns.find(column_name);
        if (it != columns.end()) {
            return it->second;
        }
        return "";
    }
    
    void set(const std::string& column_name, const std::string& value) {
        columns[column_name] = value;
    }
    
    bool has(const std::string& column_name) const {
        return columns.find(column_name) != columns.end();
    }
    
    // Serialization support
    std::vector<uint8_t> serialize() const {
        // Simple serialization: column_count + [(name_len, name, value_len, value), ...]
        std::vector<uint8_t> data;
        uint32_t col_count = columns.size();
        data.insert(data.end(), (uint8_t*)&col_count, (uint8_t*)&col_count + 4);
        
        for (const auto& [name, value] : columns) {
            uint16_t name_len = name.length();
            uint16_t val_len = value.length();
            
            data.insert(data.end(), (uint8_t*)&name_len, (uint8_t*)&name_len + 2);
            data.insert(data.end(), name.begin(), name.end());
            
            data.insert(data.end(), (uint8_t*)&val_len, (uint8_t*)&val_len + 2);
            data.insert(data.end(), value.begin(), value.end());
        }
        
        return data;
    }
    
    static Row deserialize(const std::vector<uint8_t>& data) {
        Row row;
        if (data.size() < 4) return row;
        
        size_t pos = 0;
        uint32_t col_count = *(uint32_t*)(data.data() + pos);
        pos += 4;
        
        for (uint32_t i = 0; i < col_count && pos < data.size(); i++) {
            if (pos + 2 > data.size()) break;
            uint16_t name_len = *(uint16_t*)(data.data() + pos);
            pos += 2;
            
            if (pos + name_len > data.size()) break;
            std::string name(data.begin() + pos, data.begin() + pos + name_len);
            pos += name_len;
            
            if (pos + 2 > data.size()) break;
            uint16_t val_len = *(uint16_t*)(data.data() + pos);
            pos += 2;
            
            if (pos + val_len > data.size()) break;
            std::string value(data.begin() + pos, data.begin() + pos + val_len);
            pos += val_len;
            
            row.set(name, value);
        }
        
        return row;
    }
};

}  // namespace dbx4

#endif
