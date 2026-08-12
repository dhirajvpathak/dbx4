#include <iostream>
#include "sql_parser.h"
#include <string>
#include <memory>
#include <vector>

namespace dbx4 {

std::shared_ptr<SelectStatement> parseSQL(const std::string& sql) {
    // Create empty select statement
    auto stmt = std::make_shared<SelectStatement>();
    // TODO: implement actual parsing
    return stmt;
}

}  // namespace dbx4
