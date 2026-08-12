#ifndef DBX4_QUERY_EXECUTOR_H
#define DBX4_QUERY_EXECUTOR_H

#include <string>
#include <vector>
#include <map>

namespace dbx4 {

class QueryExecutor {
public:
  std::vector<std::map<std::string, std::string>> execute_select(const std::string& table) { return {}; }
  std::vector<std::map<std::string, std::string>> execute_insert(const std::string& table, const std::vector<std::string>& cols, const std::vector<std::string>& vals) { return {}; }
  void recover_from_wal();
};

}

#endif
