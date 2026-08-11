#include "dbx4/database.h"
#include "dbx4/recovery_engine_production.cpp"
#include <iostream>
#include <fstream>
#include <cstdlib>
namespace dbx4 {
bool Database::open() {
    std::cout << "[Database] Opening " << db_path_ << "\n";
    std::string wal_dir = db_path_ + "/wal";
    system(("mkdir -p " + wal_dir).c_str());
    is_open_ = true;
    std::cout << "[Database] ✅ Opened\n";
    return true;
}
}
