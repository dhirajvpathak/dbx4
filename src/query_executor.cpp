#include "dbx4/query_executor.h"
#include <iostream>
#include <cctype>
#include <algorithm>
#include <cmath>

namespace dbx4 {

bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

std::string trim(const std::string& str) {
    size_t start = 0;
    while (start < str.length() && is_whitespace(str[start])) start++;
    size_t end = str.length();
    while (end > start && is_whitespace(str[end - 1])) end--;
    return str.substr(start, end - start);
}

std::string to_upper(const std::string& str) {
    std::string result = str;
    for (char& c : result) { c = std::toupper(c); }
    return result;
}

std::vector<std::string> split_quoted(const std::string& str, char delim) {
    std::vector<std::string> result;
    std::string current;
    bool in_quotes = false;
    for (size_t i = 0; i < str.length(); i++) {
        char c = str[i];
        if (c == '\'' && (i == 0 || str[i - 1] != '\\')) {
            in_quotes = !in_quotes;
            current += c;
        } else if (c == delim && !in_quotes) {
            result.push_back(trim(current));
            current.clear();
        } else {
            current += c;
        }
    }
    result.push_back(trim(current));
    return result;
}

std::string extract_table_name(const std::string& sql) {
    std::string upper_sql = to_upper(sql);
    size_t from_pos = upper_sql.find("FROM");
    if (from_pos == std::string::npos) throw std::runtime_error("Missing FROM");
    std::string after_from = sql.substr(from_pos + 4);
    std::string table_name = trim(after_from);
    size_t space_pos = table_name.find(' ');
    if (space_pos != std::string::npos) table_name = table_name.substr(0, space_pos);
    return table_name;
}

struct WhereClause { std::string column; std::string op; std::string value; };

WhereClause parse_where_clause(const std::string& sql) {
    size_t where_pos = sql.find(" WHERE ");
    if (where_pos == std::string::npos) return WhereClause{"", "", ""};
    std::string where_part = sql.substr(where_pos + 7);
    std::string op;
    size_t op_pos = std::string::npos;
    if ((op_pos = where_part.find("!=")) != std::string::npos) op = "!=";
    else if ((op_pos = where_part.find("<=")) != std::string::npos) op = "<=";
    else if ((op_pos = where_part.find(">=")) != std::string::npos) op = ">=";
    else if ((op_pos = where_part.find("<")) != std::string::npos) op = "<";
    else if ((op_pos = where_part.find(">")) != std::string::npos) op = ">";
    else if ((op_pos = where_part.find("=")) != std::string::npos) op = "=";
    else throw std::runtime_error("No operator");
    std::string column = trim(where_part.substr(0, op_pos));
    std::string value_part = trim(where_part.substr(op_pos + op.length()));
    size_t limit_pos = value_part.find(" LIMIT");
    if (limit_pos != std::string::npos) value_part = trim(value_part.substr(0, limit_pos));
    std::string value = value_part;
    if (!value.empty() && value.front() == '\'' && value.back() == '\'') value = value.substr(1, value.length() - 2);
    return WhereClause{column, op, value};
}

bool evaluate_where(const WhereClause& where, const std::map<std::string, std::string>& row) {
    if (where.column.empty()) return true;
    auto it = row.find(where.column);
    if (it == row.end()) throw std::runtime_error("Unknown column");
    std::string row_value = it->second;
    std::string where_value = where.value;
    try {
        double row_num = std::stod(row_value);
        double where_num = std::stod(where_value);
        if (where.op == "=") return row_num == where_num;
        if (where.op == "!=") return row_num != where_num;
        if (where.op == "<") return row_num < where_num;
        if (where.op == ">") return row_num > where_num;
        if (where.op == "<=") return row_num <= where_num;
        if (where.op == ">=") return row_num >= where_num;
    } catch (...) { }
    if (where.op == "=") return row_value == where_value;
    if (where.op == "!=") return row_value != where_value;
    if (where.op == "<") return row_value < where_value;
    if (where.op == ">") return row_value > where_value;
    if (where.op == "<=") return row_value <= where_value;
    if (where.op == ">=") return row_value >= where_value;
    return false;
}

int QueryExecutor::begin_transaction() {
    int tx_id = ++transaction_counter;
    long long ts = get_timestamp();
    active_transactions[tx_id] = TransactionContext(tx_id, ts);
    return tx_id;
}

bool QueryExecutor::commit_transaction(int tx_id) {
    if (active_transactions.find(tx_id) == active_transactions.end()) {
        throw std::runtime_error("TX not found");
    }
    
    TransactionContext& tx = active_transactions[tx_id];
    if (tx.state != TransactionState::ACTIVE) {
        throw std::runtime_error("TX not active");
    }
    
    for (auto& write : tx.write_set) {
        std::string row_key = write.first;
        std::string table_name = row_key.substr(0, row_key.find(':'));
        
        if (tables.find(table_name) != tables.end()) {
            auto& table = tables[table_name];
            VersionedRow vrow;
            vrow.data = write.second;
            vrow.version_id = tx_id;
            vrow.created_at = get_timestamp();
            vrow.deleted_at = -1;
            table.version_history[row_key].push_back(vrow);
        }
    }
    
    tx.state = TransactionState::COMMITTED;
    active_transactions.erase(tx_id);
    return true;
}

void QueryExecutor::rollback_transaction(int tx_id) {
    if (active_transactions.find(tx_id) == active_transactions.end()) {
        throw std::runtime_error("TX not found");
    }
    
    TransactionContext& tx = active_transactions[tx_id];
    tx.state = TransactionState::ABORTED;
    tx.write_set.clear();
    tx.read_set.clear();
    active_transactions.erase(tx_id);
}

std::vector<std::map<std::string, std::string>> QueryExecutor::execute(const std::string& sql) {
    std::string upper_sql = to_upper(sql);
    
    if (upper_sql.find("BEGIN") == 0) return execute_begin(sql);
    else if (upper_sql.find("COMMIT") == 0) return execute_commit(sql);
    else if (upper_sql.find("ROLLBACK") == 0) return execute_rollback(sql);
    else if (upper_sql.find("CREATE TABLE") == 0) return execute_create_table(sql);
    else if (upper_sql.find("INSERT INTO") == 0) return execute_insert(sql);
    else if (upper_sql.find("SELECT") == 0) return execute_select(sql);
    else if (upper_sql.find("UPDATE") == 0) return execute_update(sql);
    else if (upper_sql.find("DELETE") == 0) return execute_delete(sql);
    else throw std::runtime_error("Unknown SQL");
}

std::vector<std::map<std::string, std::string>> QueryExecutor::execute_begin(const std::string& sql) {
    begin_transaction();
    return {};
}

std::vector<std::map<std::string, std::string>> QueryExecutor::execute_commit(const std::string& sql) {
    if (!active_transactions.empty()) {
        int tx_id = active_transactions.rbegin()->first;
        commit_transaction(tx_id);
    }
    return {};
}

std::vector<std::map<std::string, std::string>> QueryExecutor::execute_rollback(const std::string& sql) {
    if (!active_transactions.empty()) {
        int tx_id = active_transactions.rbegin()->first;
        rollback_transaction(tx_id);
    }
    return {};
}

std::vector<std::map<std::string, std::string>> QueryExecutor::execute_create_table(const std::string& sql) {
    std::string upper_sql = to_upper(sql);
    size_t paren_start = sql.find('(');
    size_t paren_end = sql.rfind(')');
    if (paren_start == std::string::npos || paren_end == std::string::npos) throw std::runtime_error("Bad CREATE");
    
    std::string table_def = sql.substr(paren_start + 1, paren_end - paren_start - 1);
    size_t table_start = upper_sql.find("CREATE TABLE") + 12;
    std::string table_name = trim(sql.substr(table_start, paren_start - table_start));
    
    if (tables.find(table_name) != tables.end()) throw std::runtime_error("Table exists");
    
    std::vector<std::string> columns = split_quoted(table_def, ',');
    std::vector<std::string> col_names;
    for (const auto& col : columns) {
        std::string col_name = trim(col);
        size_t space_pos = col_name.find(' ');
        if (space_pos != std::string::npos) col_name = col_name.substr(0, space_pos);
        col_names.push_back(col_name);
    }
    
    Table t;
    t.name = table_name;
    t.columns = col_names;
    tables[table_name] = t;
    return {};
}

std::vector<std::map<std::string, std::string>> QueryExecutor::execute_insert(const std::string& sql) {
    std::string upper_sql = to_upper(sql);
    size_t into_pos = upper_sql.find("INTO");
    size_t values_pos = upper_sql.find("VALUES");
    if (into_pos == std::string::npos || values_pos == std::string::npos) throw std::runtime_error("Bad INSERT");
    
    std::string table_name = trim(sql.substr(into_pos + 4, values_pos - into_pos - 4));
    if (tables.find(table_name) == tables.end()) throw std::runtime_error("Table not found");
    
    Table& t = tables[table_name];
    size_t paren_start = sql.find('(', values_pos);
    size_t paren_end = sql.rfind(')');
    if (paren_start == std::string::npos || paren_end == std::string::npos) throw std::runtime_error("Bad INSERT");
    
    std::string values_str = sql.substr(paren_start + 1, paren_end - paren_start - 1);
    std::vector<std::string> values = split_quoted(values_str, ',');
    if (values.size() != t.columns.size()) throw std::runtime_error("Column mismatch");
    
    std::map<std::string, std::string> row;
    for (size_t i = 0; i < t.columns.size(); i++) {
        std::string val = trim(values[i]);
        if (!val.empty() && val.front() == '\'' && val.back() == '\'') val = val.substr(1, val.length() - 2);
        row[t.columns[i]] = val;
    }
    
    static int row_counter = 0;
    std::string row_key = table_name + ":" + std::to_string(row_counter++);
    
    VersionedRow vrow;
    vrow.data = row;
    vrow.version_id = 0;
    vrow.created_at = get_timestamp();
    vrow.deleted_at = -1;
    
    t.version_history[row_key].push_back(vrow);
    return {};
}

std::vector<std::map<std::string, std::string>> QueryExecutor::execute_select(const std::string& sql) {
    std::string upper_sql = to_upper(sql);
    std::string table_name = extract_table_name(sql);
    if (tables.find(table_name) == tables.end()) throw std::runtime_error("Table not found");
    
    Table& t = tables[table_name];
    std::vector<std::map<std::string, std::string>> result;
    WhereClause where = parse_where_clause(sql);
    
    for (auto& row_pair : t.version_history) {
        if (row_pair.second.empty()) continue;
        const VersionedRow& latest = row_pair.second.back();
        if (!latest.is_deleted() && evaluate_where(where, latest.data)) {
            result.push_back(latest.data);
        }
    }
    
    return result;
}

std::vector<std::map<std::string, std::string>> QueryExecutor::execute_update(const std::string& sql) {
    return {};
}

std::vector<std::map<std::string, std::string>> QueryExecutor::execute_delete(const std::string& sql) {
    return {};
}

}
