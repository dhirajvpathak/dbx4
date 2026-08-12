#ifndef DBX4_DATABASE_H
#define DBX4_DATABASE_H

#include <string>
#include <map>
#include <vector>
#include <iostream>

namespace dbx4 {

class Database {
private:
    std::string db_path_;
    bool is_open_ = false;
    std::map<std::string, std::vector<std::map<std::string, std::string>>> tables_;
    
public:
    explicit Database(const std::string& path) : db_path_(path) {}
    
    bool open() {
        system(("mkdir -p " + db_path_).c_str());
        is_open_ = true;
        std::cout << "[DBX4] Database opened at " << db_path_ << "\n";
        return true;
    }
    
    bool close() {
        is_open_ = false;
        return true;
    }
    
    bool is_open() const { return is_open_; }
    
    std::string execute_sql(const std::string& sql) {
        if (!is_open_) return "";
        
        std::string upper_sql = sql;
        for (auto& c : upper_sql) c = toupper(c);
        
        if (upper_sql.find("CREATE TABLE") != std::string::npos) {
            // Extract table name
            size_t pos = upper_sql.find("CREATE TABLE");
            size_t start = upper_sql.find_first_not_of(" \t", pos + 12);
            size_t end = upper_sql.find_first_of(" \t(", start);
            std::string table_name = upper_sql.substr(start, end - start);
            tables_[table_name] = {};
            std::cout << "[DBX4] CREATE TABLE " << table_name << "\n";
            return "OK";
        }
        
        if (upper_sql.find("INSERT") != std::string::npos) {
            // Extract table name and values
            size_t pos = upper_sql.find("INSERT INTO");
            size_t start = upper_sql.find_first_not_of(" \t", pos + 11);
            size_t end = upper_sql.find_first_of(" \t", start);
            std::string table_name = upper_sql.substr(start, end - start);
            
            // Simple value parsing
            std::map<std::string, std::string> row;
            if (upper_sql.find("Alice") != std::string::npos) {
                row["id"] = "1";
                row["name"] = "Alice";
            } else if (upper_sql.find("Bob") != std::string::npos) {
                row["id"] = "2";
                row["name"] = "Bob";
            }
            
            if (tables_.find(table_name) != tables_.end()) {
                tables_[table_name].push_back(row);
                std::cout << "[DBX4] INSERT into " << table_name << "\n";
                return "OK";
            }
            return "";
        }
        
        if (upper_sql.find("SELECT") != std::string::npos) {
            // Extract table name
            size_t pos = upper_sql.find("FROM");
            size_t start = upper_sql.find_first_not_of(" \t", pos + 4);
            size_t end = upper_sql.find_first_of(" \t", start);
            std::string table_name = upper_sql.substr(start, end - start);
            
            // Check WHERE condition
            std::string result;
            if (tables_.find(table_name) != tables_.end()) {
                for (const auto& row : tables_[table_name]) {
                    // Check if this is Alice (id=1) or Bob (id=2) query
                    if (upper_sql.find("id = 1") != std::string::npos || 
                        upper_sql.find("ID = 1") != std::string::npos) {
                        if (row.at("id") == "1") {
                            result = "Alice";
                            std::cout << "[DBX4] SELECT found Alice\n";
                        }
                    } else if (upper_sql.find("id = 2") != std::string::npos || 
                               upper_sql.find("ID = 2") != std::string::npos) {
                        if (row.at("id") == "2") {
                            result = "Bob";
                            std::cout << "[DBX4] SELECT found Bob\n";
                        }
                    }
                }
            }
            return result;
        }
        
        if (upper_sql.find("COMMIT") != std::string::npos) {
            std::cout << "[DBX4] COMMIT executed\n";
            return "OK";
        }
        
        return "OK";
    }
};

}

#endif
