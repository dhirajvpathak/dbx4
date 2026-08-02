#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

namespace dbx4 {

class Value {
public:
    enum Type { NULL_T, INT, DOUBLE, TEXT };
    Value() : type(NULL_T), int_val(0), double_val(0.0) {}
    explicit Value(int i) : type(INT), int_val(i), double_val(0.0) {}
    explicit Value(double d) : type(DOUBLE), int_val(0), double_val(d) {}
    explicit Value(const std::string& s) : type(TEXT), int_val(0), double_val(0.0), text_val(s) {}
    Type type;
    int int_val;
    double double_val;
    std::string text_val;
    bool is_null() const { return type == NULL_T; }
    std::string to_string() const {
        switch (type) {
            case NULL_T: return "NULL";
            case INT: return std::to_string(int_val);
            case DOUBLE: { std::string s = std::to_string(double_val); s.erase(s.find_last_not_of('0') + 1, std::string::npos); if (s.back() == '.') s.pop_back(); return s; }
            case TEXT: return text_val;
        }
        return "";
    }
};

struct Table {
    std::string name;
    std::vector<std::string> columns;
    std::vector<std::map<std::string, std::string>> rows;
};

class QueryExecutor {
public:
    QueryExecutor() = default;
    std::vector<std::map<std::string, std::string>> execute(const std::string& sql);
private:
    std::map<std::string, Table> tables;
    std::vector<std::map<std::string, std::string>> execute_create_table(const std::string& sql);
    std::vector<std::map<std::string, std::string>> execute_insert(const std::string& sql);
    std::vector<std::map<std::string, std::string>> execute_select(const std::string& sql);
    std::vector<std::map<std::string, std::string>> execute_update(const std::string& sql);
    std::vector<std::map<std::string, std::string>> execute_delete(const std::string& sql);
};

}
