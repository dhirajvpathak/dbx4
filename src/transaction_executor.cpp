#include "dbx4/transaction_log.h"
#include "dbx4/sql_parser.h"
#include <iostream>
#include <cstdint>
#include <vector>

namespace dbx4 {

class TransactionExecutor {
private:
    TransactionLog txn_log_;
    uint32_t current_txn_id_ = 0;
    bool in_transaction_ = false;
    
public:
    TransactionExecutor(const std::string& wal_dir) : txn_log_(wal_dir) {}
    
    bool open() {
        return txn_log_.open();
    }
    
    std::string execute(const std::string& sql) {
        ParsedSQL parsed = SQLParser::parse(sql);
        
        switch (parsed.type) {
            case SQLStatement::BEGIN:
                return begin_transaction();
            case SQLStatement::COMMIT:
                return commit_transaction();
            case SQLStatement::ROLLBACK:
                return rollback_transaction();
            case SQLStatement::INSERT:
                return execute_insert(sql);
            case SQLStatement::SELECT:
                return execute_select(sql);
            default:
                return "ERROR: Unknown SQL statement";
        }
    }
    
private:
    std::string begin_transaction() {
        if (in_transaction_) return "ERROR: Already in transaction";
        current_txn_id_++;
        in_transaction_ = true;
        std::cout << "[Executor] BEGIN txn " << current_txn_id_ << "\n";
        return "OK";
    }
    
    std::string commit_transaction() {
        if (!in_transaction_) return "ERROR: Not in transaction";
        
        std::vector<uint8_t> commit_marker;
        txn_log_.write_record(current_txn_id_, 4, commit_marker);
        
        in_transaction_ = false;
        std::cout << "[Executor] COMMIT txn " << current_txn_id_ << "\n";
        return "OK";
    }
    
    std::string rollback_transaction() {
        if (!in_transaction_) return "ERROR: Not in transaction";
        
        std::vector<uint8_t> abort_marker;
        txn_log_.write_record(current_txn_id_, 5, abort_marker);
        
        in_transaction_ = false;
        std::cout << "[Executor] ROLLBACK txn " << current_txn_id_ << "\n";
        return "OK";
    }
    
    std::string execute_insert(const std::string& sql) {
        if (!in_transaction_) return "ERROR: Not in transaction";
        
        std::vector<uint8_t> data(sql.begin(), sql.end());
        txn_log_.write_record(current_txn_id_, 1, data);
        
        std::cout << "[Executor] INSERT in txn " << current_txn_id_ << "\n";
        return "OK";
    }
    
    std::string execute_select(const std::string& sql) {
        std::cout << "[Executor] SELECT executed\n";
        return "[]";
    }
};

}
