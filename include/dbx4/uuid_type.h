#ifndef DBX4_UUID_TYPE_H
#define DBX4_UUID_TYPE_H

#include <string>
#include <random>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace dbx4 {

class UUID {
private:
    std::string uuid_string_;
    
public:
    UUID() {}
    
    UUID(const std::string& uuid_str) : uuid_string_(uuid_str) {}
    
    // Generate UUIDv4 (random)
    static UUID generate_v4() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        
        std::stringstream ss;
        for (int i = 0; i < 8; ++i) ss << std::hex << dis(gen);
        ss << "-";
        for (int i = 0; i < 4; ++i) ss << std::hex << dis(gen);
        ss << "-4";
        for (int i = 0; i < 3; ++i) ss << std::hex << dis(gen);
        ss << "-";
        ss << std::hex << (dis(gen) & 0x3 | 0x8);
        for (int i = 0; i < 3; ++i) ss << std::hex << dis(gen);
        ss << "-";
        for (int i = 0; i < 12; ++i) ss << std::hex << dis(gen);
        
        return UUID(ss.str());
    }
    
    std::string to_string() const {
        return uuid_string_;
    }
    
    bool is_valid() const {
        if (uuid_string_.length() != 36) return false;
        if (uuid_string_[8] != '-' || uuid_string_[13] != '-' || 
            uuid_string_[18] != '-' || uuid_string_[23] != '-') return false;
        return true;
    }
    
    bool operator==(const UUID& other) const {
        return uuid_string_ == other.uuid_string_;
    }
    
    bool operator<(const UUID& other) const {
        return uuid_string_ < other.uuid_string_;
    }
};

}

#endif
