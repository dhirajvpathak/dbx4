#pragma once

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <memory>
#include <fstream>
#include <sstream>

namespace dbx4 {

enum class OpType {
    INSERT,
    UPDATE,
    DELETE,
    COMMIT,
    ROLLBACK
};

struct LogEntry {
    long long timestamp;
    int tx_id;
    OpType op;
    std::string table_name;
    std::map<std::string, std::string> row_data;
    bool committed;
    
    LogEntry() : timestamp(0), tx_id(0), op(OpType::INSERT), committed(false) {}
    LogEntry(long long ts, int id, OpType o, const std::string& t, const std::map<std::string, std::string>& d)
        : timestamp(ts), tx_id(id), op(o), table_name(t), row_data(d), committed(false) {}
};

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

struct VersionedRow {
    std::map<std::string, std::string> data;
    int version_id;
    long long created_at;
    long long deleted_at;
    bool is_deleted() const { return deleted_at >= 0; }
};

enum class TransactionState {
    ACTIVE,
    COMMITTED,
    ABORTED
};

struct TransactionContext {
    int tx_id;
    long long start_time;
    std::vector<std::string> read_set;
    std::map<std::string, std::map<std::string, std::string>> write_set;
    TransactionState state;
    
    TransactionContext() : tx_id(0), start_time(0), state(TransactionState::ACTIVE) {}
    TransactionContext(int id, long long time) 
        : tx_id(id), start_time(time), state(TransactionState::ACTIVE) {}
};

struct Table {
    std::string name;
    std::vector<std::string> columns;
    std::map<std::string, std::vector<VersionedRow>> version_history;
};

class WalManager {
public:
    WalManager(const std::string& log_dir = "/tmp/dbx4_wal") : log_directory(log_dir) {}
    
    void write_log_entry(const LogEntry& entry);
    std::vector<LogEntry> read_wal(const std::string& table_name);
    void flush_wal();
    void clear_wal(const std::string& table_name);
    
private:
    std::string log_directory;
    std::map<std::string, std::vector<LogEntry>> pending_writes;
};

class RecoveryManager {
public:
    RecoveryManager(WalManager& wal) : wal_manager(wal) {}
    
    void recover(std::map<std::string, Table>& tables);
    
private:
    WalManager& wal_manager;
};

class QueryExecutor {
public:
    QueryExecutor(const std::string& log_dir = "/tmp/dbx4_wal") 
        : transaction_counter(0), clock(0), wal_manager(log_dir), recovery_manager(wal_manager) {
        recover_from_wal();
    }
    
    std::vector<std::map<std::string, std::string>> execute(const std::string& sql);
    
private:
    std::map<std::string, Table> tables;
    std::map<int, TransactionContext> active_transactions;
    int transaction_counter;
    long long clock;
    WalManager wal_manager;
    RecoveryManager recovery_manager;
    
    long long get_timestamp() { return ++clock; }
    int begin_transaction();
    bool commit_transaction(int tx_id);
    void rollback_transaction(int tx_id);
    void recover_from_wal();
    
    std::vector<std::map<std::string, std::string>> execute_create_table(const std::string& sql);
    std::vector<std::map<std::string, std::string>> execute_insert(const std::string& sql);
    std::vector<std::map<std::string, std::string>> execute_select(const std::string& sql);
    std::vector<std::map<std::string, std::string>> execute_update(const std::string& sql);
    std::vector<std::map<std::string, std::string>> execute_delete(const std::string& sql);
    std::vector<std::map<std::string, std::string>> execute_begin(const std::string& sql);
    std::vector<std::map<std::string, std::string>> execute_commit(const std::string& sql);
    std::vector<std::map<std::string, std::string>> execute_rollback(const std::string& sql);
};

}
