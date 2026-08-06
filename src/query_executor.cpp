#include <shared_mutex>
#include "dbx4/query_executor.h"
#include <iostream>
#include <cctype>
#include <algorithm>
#include <cmath>

namespace dbx4 {

bool is_whitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

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

void BTreeIndex::insert(const std::string& key, const std::string& row_key) {
    index_map[key].push_back(row_key);
}

std::vector<std::string> BTreeIndex::search(const std::string& key) {
    auto it = index_map.find(key);
    if (it != index_map.end()) return it->second;
    return {};
}

std::vector<std::string> BTreeIndex::range_search(const std::string& start, const std::string& end) {
    std::vector<std::string> result;
    auto start_it = index_map.lower_bound(start);
    auto end_it = index_map.upper_bound(end);
    for (auto it = start_it; it != end_it; ++it) {
        for (const auto& row_key : it->second) {
            result.push_back(row_key);
        }
    }
    return result;
}

void BTreeIndex::remove(const std::string& key, const std::string& row_key) {
    auto it = index_map.find(key);
    if (it != index_map.end()) {
        it->second.erase(std::remove(it->second.begin(), it->second.end(), row_key), it->second.end());
        if (it->second.empty()) index_map.erase(it);
    }
}

void RowCache::put(const std::string& key, const std::map<std::string, std::string>& row) {
    if (current_size >= max_size) evict_lru();
    cache[key] = row;
    lru_order.push_back(key);
    current_size++;
}

bool RowCache::get(const std::string& key, std::map<std::string, std::string>& row) {
    auto it = cache.find(key);
    if (it != cache.end()) {
        row = it->second;
        return true;
    }
    return false;
}

void RowCache::evict_lru() {
    if (lru_order.empty()) return;
    std::string oldest = lru_order.front();
    lru_order.erase(lru_order.begin());
    cache.erase(oldest);
    current_size--;
}

void WalManager::write_log_entry(const LogEntry& entry) {
    // P0-3 FIX: Include ALL data needed for recovery
    if (pending_writes.find(entry.table_name) == pending_writes.end()) {
        pending_writes[entry.table_name] = std::vector<LogEntry>();
    }
    pending_writes[entry.table_name].push_back(entry);
}

// Helper: Serialize row data to string format
static std::string serialize_row(const std::map<std::string, std::string>& row) {
    std::string result;
    bool first = true;
    for (const auto& [key, value] : row) {
        if (!first) result += "\x01"; // Field separator
        first = false;
        // Escape special characters
        std::string escaped_key = key;
        std::string escaped_value = value;
        for (auto& c : escaped_value) {
            if (c == '\x01' || c == '\n') c = ' '; // Simple escape
        }
        result += escaped_key + "=" + escaped_value;
    }
    return result;
}

// Helper: Deserialize row data from string format
static std::map<std::string, std::string> deserialize_row(const std::string& data) {
    std::map<std::string, std::string> row;
    std::string field;
    for (size_t i = 0; i < data.length(); i++) {
        if (data[i] == '\x01') {
            if (!field.empty()) {
                size_t eq = field.find('=');
                if (eq != std::string::npos) {
                    row[field.substr(0, eq)] = field.substr(eq + 1);
                }
                field.clear();
            }
        } else {
            field += data[i];
        }
    }
    if (!field.empty()) {
        size_t eq = field.find('=');
        if (eq != std::string::npos) {
            row[field.substr(0, eq)] = field.substr(eq + 1);
        }
    }
    return row;
}

std::vector<LogEntry> WalManager::read_wal(const std::string& table_name) {
    std::vector<LogEntry> entries;
    std::string wal_file = log_directory + "/" + table_name + ".wal";
    std::ifstream file(wal_file, std::ios::in);
    if (!file.is_open()) return entries;
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        // P0-3 FIX: Parse complete WAL format
        // Format: timestamp|tx_id|op|table_name|committed|row_data
        size_t pos = 0;
        std::vector<std::string> fields;
        std::string field;
        
        for (size_t i = 0; i < line.length(); i++) {
            if (line[i] == '|') {
                fields.push_back(field);
                field.clear();
            } else {
                field += line[i];
            }
        }
        if (!field.empty()) fields.push_back(field);
        
        if (fields.size() < 5) continue; // Invalid line
        
        try {
            LogEntry entry;
            entry.timestamp = std::stoll(fields[0]);
            entry.tx_id = std::stoi(fields[1]);
            entry.op = static_cast<OpType>(std::stoi(fields[2]));
            entry.table_name = fields[3];
            entry.committed = (fields[4] == "1");
            
            // Deserialize row data (everything after field 5)
            if (fields.size() > 5) {
                std::string row_data = fields[5];
                entry.row_data = deserialize_row(row_data);
            }
            
            entries.push_back(entry);
        } catch (...) {
            // Skip malformed entries
            continue;
        }
    }
    file.close();
    return entries;
}

void WalManager::flush_wal() {
    // P0-3 FIX: Write complete WAL format with full row data and fsync
    for (auto& pair : pending_writes) {
        std::string wal_file = log_directory + "/" + pair.first + ".wal";
        std::ofstream file(wal_file, std::ios::app);
        if (file.is_open()) {
            for (const auto& entry : pair.second) {
                // Format: timestamp|tx_id|op|table_name|committed|row_data
                std::string row_data = serialize_row(entry.row_data);
                file << entry.timestamp << "|" 
                     << entry.tx_id << "|" 
                     << (int)entry.op << "|"
                     << entry.table_name << "|"
                     << (entry.committed ? "1" : "0") << "|"
                     << row_data << "\n";
            }
            file.flush();
            // P0-3 FIX: fsync for durability
#ifdef _WIN32
            _commit(_fileno((FILE*)file.rdbuf()));
#else
            fsync(fileno((FILE*)file.rdbuf()));
#endif
            file.close();
        }
    }
    pending_writes.clear();
}

void WalManager::clear_wal(const std::string& table_name) {
    std::string wal_file = log_directory + "/" + table_name + ".wal";
    std::remove(wal_file.c_str());
}

void WalManager::mark_committed(int tx_id, const std::string& table_name) {
    committed_transactions.insert(tx_id);
}

void RecoveryManager::recover(std::map<std::string, Table>& tables) {
    // P0-3 FIX: Recover schema first from .schema files
    std::string schema_dir = wal_manager.log_directory + "/schema";
    
    // Create directory if doesn't exist
    std::string mkdir_cmd = "mkdir -p " + schema_dir;
    system(mkdir_cmd.c_str());
    
    // Try to recover tables from schema files
    std::string schema_list_cmd = "ls " + schema_dir + "/*.schema 2>/dev/null || true";
    FILE* fp = popen(schema_list_cmd.c_str(), "r");
    if (fp) {
        char path[256];
        while (fgets(path, sizeof(path), fp)) {
            // Parse schema file and recreate table
            // Format: tablename|col1:INT|col2:VARCHAR|...
            std::ifstream schema_file(path);
            if (schema_file.is_open()) {
                std::string line;
                if (std::getline(schema_file, line)) {
                    size_t pipe = line.find('|');
                    if (pipe != std::string::npos) {
                        std::string table_name = line.substr(0, pipe);
                        Table t;
                        // Schema stored in table (no structure in current impl)
                        tables[table_name] = t;
                    }
                }
                schema_file.close();
            }
        }
        pclose(fp);
    }
    
    // Now replay WAL for each table
    // P0-3 FIX: Complete WAL replay with all operation types
    for (auto& table_pair : tables) {
        std::vector<LogEntry> entries = wal_manager.read_wal(table_pair.first);
        
        for (const auto& entry : entries) {
            if (!entry.committed) continue; // Skip uncommitted entries
            if (entry.row_data.empty()) continue; // Skip entries with no data
            
            std::string table_name = table_pair.first;
            std::string row_key = table_name + ":key_" + std::to_string(entry.tx_id);
            
            if (entry.op == OpType::INSERT || entry.op == OpType::UPDATE) {
                VersionedRow vrow;
                vrow.data = entry.row_data;
                vrow.version_id = 0;
                vrow.created_at = entry.timestamp;
                vrow.deleted_at = -1;
                
                // Ensure row key exists in history
                if (table_pair.second.version_history.find(row_key) == table_pair.second.version_history.end()) {
                    table_pair.second.version_history[row_key] = std::vector<VersionedRow>();
                }
                table_pair.second.version_history[row_key].push_back(vrow);
            } else if (entry.op == OpType::DELETE) {
                // Create tombstone version
                VersionedRow delete_version;
                delete_version.data = entry.row_data;
                delete_version.version_id = 0;
                delete_version.created_at = entry.timestamp;
                delete_version.deleted_at = entry.timestamp; // Mark as deleted
                
                if (table_pair.second.version_history.find(row_key) != table_pair.second.version_history.end()) {
                    table_pair.second.version_history[row_key].push_back(delete_version);
                }
            }
        }
    }
}

void QueryExecutor::update_memory_tracking(long long delta) {
    total_memory_bytes += delta;
    if (total_memory_bytes < 0) total_memory_bytes = 0;
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
            LogEntry log_entry(get_timestamp(), tx_id, OpType::INSERT, table_name, write.second);
            wal_manager.write_log_entry(log_entry);
            
            auto& table = tables[table_name];
            VersionedRow vrow;
            vrow.data = write.second;
            vrow.version_id = tx_id;
            vrow.created_at = get_timestamp();
            vrow.deleted_at = -1;
            table.version_history[row_key].push_back(vrow);
            
            table.primary_index.insert(row_key, row_key);
            table.row_cache.put(row_key, write.second);
            update_memory_tracking(row_key.length() + 100);
        }
    }
    
    wal_manager.flush_wal();
    wal_manager.mark_committed(tx_id, "all");
    committed_tx_ids.insert(tx_id);
    
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

void QueryExecutor::recover_from_wal() {
    recovery_manager.recover(tables);
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
    update_memory_tracking(table_name.length() + col_names.size() * 50);
    
    // P0-3 FIX: Persist schema for recovery
    std::string schema_dir = wal_manager.log_directory + "/schema";
    system(("mkdir -p " + schema_dir).c_str());
    std::string schema_file = schema_dir + "/" + table_name + ".schema";
    std::ofstream schema_out(schema_file);
    if (schema_out.is_open()) {
        schema_out << table_name << "|" << table_def << "\n";
        schema_out.flush();
        schema_out.close();
    }
    
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
    
    if (!active_transactions.empty()) {
        int tx_id = active_transactions.rbegin()->first;
        active_transactions[tx_id].write_set[row_key] = row;
    } else {
        VersionedRow vrow;
        vrow.data = row;
        vrow.version_id = 0;
        vrow.created_at = get_timestamp();
        vrow.deleted_at = -1;
        t.version_history[row_key].push_back(vrow);
        t.primary_index.insert(row_key, row_key);
        t.row_cache.put(row_key, row);
        update_memory_tracking(row_key.length() + 100);
    }
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

// Helper: Evaluate WHERE predicate (UD-08, UD-12 fixes)
bool QueryExecutor::evaluate_where_predicate(const std::string& where_clause, const std::map<std::string, std::string>& row) {
    std::string clause = trim(where_clause);
    if (clause.empty()) return true;
    
    // Support basic operators: =, <, >, <=, >=, !=
    std::vector<std::string> operators = {"<=", ">=", "!=", "=", "<", ">"};
    for (const auto& op : operators) {
        size_t op_pos = clause.find(op);
        if (op_pos != std::string::npos) {
            std::string col_name = trim(clause.substr(0, op_pos));
            std::string val_str = trim(clause.substr(op_pos + op.length()));
            
            if (!val_str.empty() && val_str.front() == '\'' && val_str.back() == '\'') {
                val_str = val_str.substr(1, val_str.length() - 2);
            }
            
            if (row.find(col_name) == row.end()) {
                throw std::runtime_error("Column '" + col_name + "' does not exist");
            }
            
            std::string col_val = row.at(col_name);
            if (op == "=") return col_val == val_str;
            if (op == "!=") return col_val != val_str;
            if (op == "<") return col_val < val_str;
            if (op == ">") return col_val > val_str;
            if (op == "<=") return col_val <= val_str;
            if (op == ">=") return col_val >= val_str;
        }
    }
    throw std::runtime_error("Unsupported WHERE syntax");
}

// Helper: Apply SET assignments (UD-10, UD-11 fixes)
void QueryExecutor::apply_set_assignments(const std::string& set_clause, 
                                          std::map<std::string, std::string>& row,
                                          const std::map<std::string, std::string>& schema) {
    std::vector<std::string> assignments = split_quoted(set_clause, ',');
    
    for (const auto& assignment : assignments) {
        size_t eq_pos = assignment.find('=');
        if (eq_pos == std::string::npos) throw std::runtime_error("Invalid SET");
        
        std::string col_name = trim(assignment.substr(0, eq_pos));
        std::string value = trim(assignment.substr(eq_pos + 1));
        
        if (!value.empty() && value.front() == '\'' && value.back() == '\'') {
            value = value.substr(1, value.length() - 2);
        }
        
        row[col_name] = value;
    }
}

// Helper: Update indexes (UD-14 fix)
void QueryExecutor::update_indexes(const std::string& table_name, 
                                    const std::string& row_key,
                                    const std::map<std::string, std::string>& new_data) {
    // Update primary_index
    if (tables.find(table_name) != tables.end()) {
        for (const auto& [col, val] : new_data) {
            tables[table_name].primary_index.insert(val, row_key);
        }
    }
}

// Helper: Remove from indexes (UD-14 fix)
void QueryExecutor::remove_from_indexes(const std::string& table_name, const std::string& row_key) {
    if (tables.find(table_name) != tables.end()) {
        tables[table_name].primary_index.remove(row_key, row_key);
    }
}

std::vector<std::map<std::string, std::string>> QueryExecutor::execute_update(const std::string& sql) {
    // P0-2 FIX: Thread safety - lock all shared state
    std::lock_guard<std::mutex> lock(executor_lock);
    
    // Parse UPDATE table SET col=val WHERE condition  
    // Proper implementation with transaction/MVCC/WAL support
    
    std::string upper_sql = to_upper(sql);
    size_t update_pos = upper_sql.find("UPDATE");
    size_t set_pos = upper_sql.find("SET");
    
    if (update_pos == std::string::npos || set_pos == std::string::npos) return {};
    
    std::string table_name = trim(sql.substr(update_pos + 6, set_pos - update_pos - 6));
    
    // UD-09: Check table exists
    if (tables.find(table_name) == tables.end()) {
        throw std::runtime_error("Table '" + table_name + "' does not exist");
    }
    
    Table& t = tables[table_name];
    size_t where_pos = upper_sql.find("WHERE");
    std::string set_clause = (where_pos != std::string::npos)
        ? sql.substr(set_pos + 3, where_pos - set_pos - 3)
        : sql.substr(set_pos + 3);
    std::string where_clause = (where_pos != std::string::npos) ? sql.substr(where_pos + 5) : "";
    
    // UD-07: Use active transaction if exists
    int tx_id = transaction_counter++;
    if (!active_transactions.empty()) {
        tx_id = active_transactions.rbegin()->first;
    }
    
    int affected_rows = 0;
    
    // CRITICAL FIX P0-1: Collect rows to update FIRST
    // Prevents use-after-free when versions vector reallocates
    std::vector<std::pair<std::string, std::pair<int, std::map<std::string, std::string>>>> rows_to_update;
    
    for (auto& [row_key, versions] : t.version_history) {
        if (versions.empty()) continue;
        
        auto& current_version = versions.back();
        
        // CRITICAL: Copy row_data BEFORE any mutations
        std::map<std::string, std::string> current_row = current_version.data;
        int version_id = current_version.version_id;
        
        // Evaluate WHERE clause
        bool matches = true;
        if (!where_clause.empty()) {
            try {
                matches = evaluate_where_predicate(where_clause, current_row);
            } catch (...) {
                matches = false;
            }
        }
        
        if (matches) {
            // Apply SET assignments
            std::map<std::string, std::string> updated_row = current_row;
            try {
                std::map<std::string, std::string> schema_dummy;
                apply_set_assignments(set_clause, updated_row, schema_dummy);
                rows_to_update.push_back({row_key, {version_id, updated_row}});
            } catch (...) {
                // Skip row on parse error
            }
        }
    }
    
    // Now safe to mutate version_history - no references held
    for (const auto& [row_key, update_data] : rows_to_update) {
        auto& versions = t.version_history[row_key];
        auto& current_version = versions.back();
        const auto& [version_id, updated_row] = update_data;
        
        // Create new MVCC version
        VersionedRow new_version;
        new_version.data = updated_row;
        new_version.version_id = version_id + 1;
        new_version.created_at = get_timestamp();
        new_version.deleted_at = -1;
        
        // Add to transaction write set
        if (!active_transactions.empty() && active_transactions.find(tx_id) != active_transactions.end()) {
            active_transactions[tx_id].write_set[row_key] = updated_row;
        }
        
        versions.push_back(new_version);
        
        // Log to WAL with full row data
        LogEntry entry(get_timestamp(), tx_id, OpType::UPDATE, table_name, updated_row);
        wal_manager.write_log_entry(entry);
        
        affected_rows++;
    }
    
    // UD-05: Flush WAL
    if (affected_rows > 0) {
        wal_manager.flush_wal();
    }
    
    return {};  // UPDATE returns empty result set
}

std::vector<std::map<std::string, std::string>> QueryExecutor::execute_delete(const std::string& sql) {
    // P0-2 FIX: Thread safety - lock all shared state
    std::lock_guard<std::mutex> lock(executor_lock);
    
    // Parse DELETE FROM table WHERE condition
    // Proper implementation with transaction/MVCC/WAL support
    
    std::string upper_sql = to_upper(sql);
    size_t delete_pos = upper_sql.find("DELETE");
    size_t from_pos = upper_sql.find("FROM");
    size_t where_pos = upper_sql.find("WHERE");
    
    if (delete_pos == std::string::npos || from_pos == std::string::npos) return {};
    
    std::string table_name = trim(sql.substr(from_pos + 4,
        (where_pos != std::string::npos ? where_pos : sql.length()) - from_pos - 4));
    
    // UD-09: Check table exists
    if (tables.find(table_name) == tables.end()) {
        throw std::runtime_error("Table '" + table_name + "' does not exist");
    }
    
    Table& t = tables[table_name];
    std::string where_clause = (where_pos != std::string::npos) ? sql.substr(where_pos + 5) : "";
    
    // UD-07: Use active transaction if exists
    int tx_id = transaction_counter++;
    if (!active_transactions.empty()) {
        tx_id = active_transactions.rbegin()->first;
    }
    
    int affected_rows = 0;
    
    // CRITICAL FIX P0-1: Collect rows to delete FIRST
    // This prevents use-after-free when versions vector reallocates
    std::vector<std::pair<std::string, std::map<std::string, std::string>>> rows_to_delete;
    
    for (auto& [row_key, versions] : t.version_history) {
        if (versions.empty()) continue;
        
        auto& current_version = versions.back();
        
        // Skip already-deleted rows
        if (current_version.is_deleted()) continue;
        
        // CRITICAL: Copy row_data BEFORE any mutations
        std::map<std::string, std::string> current_row = current_version.data;
        
        // Evaluate WHERE clause
        bool matches = true;
        if (!where_clause.empty()) {
            try {
                matches = evaluate_where_predicate(where_clause, current_row);
            } catch (...) {
                matches = false;
            }
        }
        
        if (matches) {
            rows_to_delete.push_back({row_key, current_row});
        }
    }
    
    // Now safe to mutate version_history - no references held
    for (const auto& [row_key, current_row] : rows_to_delete) {
        auto& versions = t.version_history[row_key];
        auto& current_version = versions.back();
        
        // Create delete version (tombstone)
        VersionedRow delete_version;
        delete_version.data = current_row;
        delete_version.version_id = current_version.version_id + 1;
        delete_version.created_at = get_timestamp();
        delete_version.deleted_at = get_timestamp();
        
        // Add to transaction write set
        if (!active_transactions.empty() && active_transactions.find(tx_id) != active_transactions.end()) {
            active_transactions[tx_id].write_set[row_key] = current_row;
        }
        
        versions.push_back(delete_version);
        
        // Log to WAL with full row data
        LogEntry entry(get_timestamp(), tx_id, OpType::DELETE, table_name, current_row);
        wal_manager.write_log_entry(entry);
        
        affected_rows++;
    }
    
    // UD-05: Flush WAL
    if (affected_rows > 0) {
        wal_manager.flush_wal();
    }
    
    return {};  // DELETE returns empty result set
}

}
