#ifndef DBX4_STORAGE_ENGINE_H
#define DBX4_STORAGE_ENGINE_H
#include <string>
#include <vector>
#include <map>
namespace dbx4 {
class StorageEngine {
public:
    bool create_table(const std::string& name) { return true; }
    bool insert_row(const std::string& table, const std::map<std::string, std::string>& row) { return true; }
};
}
#endif
