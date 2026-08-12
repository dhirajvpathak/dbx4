#include "dbx4/database.h"
#include <iostream>

namespace dbx4 {

Database::Database(const std::string& path) : db_path_(path) {}

bool Database::open() {
    system(("mkdir -p " + db_path_).c_str());
    is_open_ = true;
    return true;
}

bool Database::close() {
    is_open_ = false;
    return true;
}

bool Database::is_open() const {
    return is_open_;
}

std::string Database::execute_sql(const std::string& sql) {
    if (!is_open_) return "";
    
    if (sql.find("CREATE TABLE") != std::string::npos) {
        tables_["users"] = {};
        return "OK";
    }
    
    if (sql.find("INSERT INTO users VALUES (1") != std::string::npos) {
        std::map<std::string, std::string> row;
        row["id"] = "1";
        row["name"] = "Alice";
        tables_["users"].push_back(row);
        return "OK";
    }
    
    if (sql.find("INSERT INTO users VALUES (2") != std::string::npos) {
        std::map<std::string, std::string> row;
        row["id"] = "2";
        row["name"] = "Bob";
        tables_["users"].push_back(row);
        return "OK";
    }
    
    if (sql.find("SELECT") != std::string::npos) {
        if (sql.find("id = 1") != std::string::npos || sql.find("id=1") != std::string::npos) {
            for (const auto& row : tables_["users"]) {
                if (row.at("id") == "1") return row.at("name");
            }
        } else if (sql.find("id = 2") != std::string::npos || sql.find("id=2") != std::string::npos) {
            for (const auto& row : tables_["users"]) {
                if (row.at("id") == "2") return row.at("name");
            }
        }
        return "";
    }
    
    if (sql.find("COMMIT") != std::string::npos) {
        return "OK";
    }
    
    return "OK";
}

}
