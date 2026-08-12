#ifndef DBX4_JSON_TYPE_H
#define DBX4_JSON_TYPE_H

#include <string>
#include <map>
#include <vector>
#include <iostream>

namespace dbx4 {

class JSONValue {
private:
    std::string raw_json_;
    std::map<std::string, std::string> parsed_;
    
public:
    JSONValue() {}
    
    JSONValue(const std::string& json) : raw_json_(json) {
        parse_json();
    }
    
    void parse_json() {
        // Simple JSON parser for key-value pairs
        size_t pos = 0;
        while ((pos = raw_json_.find("\"", pos)) != std::string::npos) {
            size_t key_start = pos + 1;
            size_t key_end = raw_json_.find("\"", key_start);
            if (key_end == std::string::npos) break;
            
            std::string key = raw_json_.substr(key_start, key_end - key_start);
            
            size_t colon = raw_json_.find(":", key_end);
            if (colon == std::string::npos) break;
            
            size_t val_start = raw_json_.find("\"", colon);
            if (val_start == std::string::npos) {
                // Handle numeric values
                val_start = colon + 1;
                while (val_start < raw_json_.length() && (raw_json_[val_start] == ' ' || raw_json_[val_start] == ':')) {
                    val_start++;
                }
                size_t val_end = raw_json_.find_first_of(",}", val_start);
                if (val_end != std::string::npos) {
                    std::string val = raw_json_.substr(val_start, val_end - val_start);
                    // Trim whitespace
                    val.erase(0, val.find_first_not_of(" \t\n\r"));
                    val.erase(val.find_last_not_of(" \t\n\r") + 1);
                    parsed_[key] = val;
                }
            } else {
                val_start++;
                size_t val_end = raw_json_.find("\"", val_start);
                if (val_end != std::string::npos) {
                    std::string val = raw_json_.substr(val_start, val_end - val_start);
                    parsed_[key] = val;
                }
            }
            
            pos = key_end + 1;
        }
    }
    
    // -> operator: get value as JSON
    std::string operator_arrow(const std::string& key) const {
        auto it = parsed_.find(key);
        if (it != parsed_.end()) {
            return "\"" + it->second + "\"";
        }
        return "null";
    }
    
    // ->> operator: get value as text (without quotes)
    std::string operator_arrow_arrow(const std::string& key) const {
        auto it = parsed_.find(key);
        if (it != parsed_.end()) {
            return it->second;
        }
        return "null";
    }
    
    // @> operator: contains JSON object
    bool operator_contains(const std::string& other_json) const {
        JSONValue other(other_json);
        for (const auto& [key, val] : other.parsed_) {
            auto it = parsed_.find(key);
            if (it == parsed_.end() || it->second != val) {
                return false;
            }
        }
        return true;
    }
    
    // ? operator: contains key
    bool operator_contains_key(const std::string& key) const {
        return parsed_.find(key) != parsed_.end();
    }
    
    std::string to_string() const {
        return raw_json_;
    }
    
    std::string get_value(const std::string& key) const {
        auto it = parsed_.find(key);
        if (it != parsed_.end()) {
            return it->second;
        }
        return "";
    }
};

}

#endif
