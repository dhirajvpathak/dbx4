#ifndef DBX4_DATABASE_H
#define DBX4_DATABASE_H
#include "dbx4/wal_format.h"
#include "dbx4/transaction_api.h"
#include <string>
#include <map>
#include <vector>
#include <memory>
namespace dbx4 {
class Database {
private:
    std::string db_path_;
    WALConfig wal_config_;
    std::map<std::string, std::vector<std::map<std::string, std::string>>> tables_;
    bool is_open_ = false;
public:
    Database(const std::string& path) : db_path_(path) {
        wal_config_.wal_dir = db_path_ + "/wal";
    }
    bool open() {
        std::cout << "[Database] Opening " << db_path_ << "\n";
        is_open_ = true;
        return true;
    }
    bool close() {
        is_open_ = false;
        return true;
    }
    bool is_open() const { return is_open_; }
    std::string execute_sql(const std::string& sql) {
        if (!is_open_) return "ERROR: Database not open";
        return "OK";
    }
};
}
#endif
