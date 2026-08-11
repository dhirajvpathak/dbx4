#ifndef DBX4_SQL_PARSER_H
#define DBX4_SQL_PARSER_H
#include <string>
#include <vector>
#include <map>
namespace dbx4 {
enum class SQLStatement { BEGIN, COMMIT, ROLLBACK, INSERT, SELECT, UPDATE, DELETE, CREATE_TABLE, UNKNOWN };
struct ParsedSQL {
    SQLStatement type;
    std::string table_name;
    std::vector<std::string> columns;
    std::vector<std::string> values;
    std::string where_clause;
};
class SQLParser {
public:
    static ParsedSQL parse(const std::string& sql) {
        ParsedSQL parsed;
        std::string upper_sql = sql;
        for (auto& c : upper_sql) c = toupper(c);
        if (upper_sql.find("BEGIN") != std::string::npos) parsed.type = SQLStatement::BEGIN;
        else if (upper_sql.find("COMMIT") != std::string::npos) parsed.type = SQLStatement::COMMIT;
        else if (upper_sql.find("ROLLBACK") != std::string::npos) parsed.type = SQLStatement::ROLLBACK;
        else if (upper_sql.find("INSERT") != std::string::npos) parsed.type = SQLStatement::INSERT;
        else if (upper_sql.find("SELECT") != std::string::npos) parsed.type = SQLStatement::SELECT;
        else if (upper_sql.find("UPDATE") != std::string::npos) parsed.type = SQLStatement::UPDATE;
        else if (upper_sql.find("DELETE") != std::string::npos) parsed.type = SQLStatement::DELETE;
        else if (upper_sql.find("CREATE TABLE") != std::string::npos) parsed.type = SQLStatement::CREATE_TABLE;
        else parsed.type = SQLStatement::UNKNOWN;
        return parsed;
    }
};
}
#endif
