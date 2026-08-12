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
    Database(const std::string& path);
    
    bool open();
    bool close();
    bool is_open() const;
    std::string execute_sql(const std::string& sql);
};

}

#endif
