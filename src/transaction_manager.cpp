#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <set>

namespace dbx4 {

struct VersionedRow {
    int version_id = 0;
    bool is_deleted = false;
    std::map<std::string, std::string> data;
};

struct UndoLogEntry {
    std::string table_name;
    std::string row_id;
    VersionedRow old_version;
    std::string operation;  // INSERT, UPDATE, DELETE
};

struct Transaction {
    int txn_id = 0;
    bool active = false;
    bool committed = false;
    
    // Transaction-local state
    std::map<std::string, VersionedRow> local_writes;  // Read-own-writes
    std::vector<UndoLogEntry> undo_log;                // For rollback
    std::set<std::string> read_set;                    // For validation
    std::set<std::string> write_set;
};

class TransactionManager {
private:
    std::map<int, Transaction> transactions;
    std::mutex txn_lock;
    
public:
    // Begin transaction
    bool begin(int txn_id) {
        std::lock_guard<std::mutex> lock(txn_lock);
        
        if (transactions.count(txn_id)) return false;
        
        Transaction& txn = transactions[txn_id];
        txn.txn_id = txn_id;
        txn.active = true;
        txn.committed = false;
        txn.local_writes.clear();
        txn.undo_log.clear();
        txn.read_set.clear();
        txn.write_set.clear();
        
        return true;
    }
    
    // INSERT: transaction-local, with undo
    bool insert(int txn_id, const std::string& table, const std::string& row_id,
               const VersionedRow& row) {
        std::lock_guard<std::mutex> lock(txn_lock);
        
        if (!transactions.count(txn_id)) return false;
        Transaction& txn = transactions[txn_id];
        
        // Record undo (for rollback)
        UndoLogEntry undo;
        undo.table_name = table;
        undo.row_id = row_id;
        undo.operation = "INSERT";
        undo.old_version.is_deleted = true;  // Undo: delete this row
        txn.undo_log.push_back(undo);
        
        // Make locally visible (read-your-own-writes)
        txn.local_writes[table + ":" + row_id] = row;
        txn.write_set.insert(table + ":" + row_id);
        
        return true;
    }
    
    // UPDATE: transaction-local, with undo of old version
    bool update(int txn_id, const std::string& table, const std::string& row_id,
               const VersionedRow& old_version, const VersionedRow& new_version) {
        std::lock_guard<std::mutex> lock(txn_lock);
        
        if (!transactions.count(txn_id)) return false;
        Transaction& txn = transactions[txn_id];
        
        // Record undo (for rollback)
        UndoLogEntry undo;
        undo.table_name = table;
        undo.row_id = row_id;
        undo.operation = "UPDATE";
        undo.old_version = old_version;  // Undo: restore old version
        txn.undo_log.push_back(undo);
        
        // Make locally visible (read-your-own-writes)
        txn.local_writes[table + ":" + row_id] = new_version;
        txn.write_set.insert(table + ":" + row_id);
        
        return true;
    }
    
    // DELETE: transaction-local, with undo to restore row
    bool delete_row(int txn_id, const std::string& table, const std::string& row_id,
                   const VersionedRow& old_version) {
        std::lock_guard<std::mutex> lock(txn_lock);
        
        if (!transactions.count(txn_id)) return false;
        Transaction& txn = transactions[txn_id];
        
        // Record undo (for rollback)
        UndoLogEntry undo;
        undo.table_name = table;
        undo.row_id = row_id;
        undo.operation = "DELETE";
        undo.old_version = old_version;  // Undo: restore deleted row
        txn.undo_log.push_back(undo);
        
        // Mark as deleted locally
        VersionedRow deleted_version = old_version;
        deleted_version.is_deleted = true;
        txn.local_writes[table + ":" + row_id] = deleted_version;
        txn.write_set.insert(table + ":" + row_id);
        
        return true;
    }
    
    // Read from transaction-local write set (read-your-own-writes)
    bool read(int txn_id, const std::string& table, const std::string& row_id,
             VersionedRow& out_row) {
        std::lock_guard<std::mutex> lock(txn_lock);
        
        if (!transactions.count(txn_id)) return false;
        Transaction& txn = transactions[txn_id];
        
        // Check local writes first
        std::string key = table + ":" + row_id;
        if (txn.local_writes.count(key)) {
            out_row = txn.local_writes[key];
            txn.read_set.insert(key);
            return true;
        }
        
        // (Real implementation would check persistent storage here)
        txn.read_set.insert(key);
        return false;
    }
    
    // Commit: make all local changes permanent
    bool commit(int txn_id) {
        std::lock_guard<std::mutex> lock(txn_lock);
        
        if (!transactions.count(txn_id)) return false;
        Transaction& txn = transactions[txn_id];
        
        // (Real implementation would:
        //  1. Validate read set (serializability)
        //  2. Write changes to persistent storage
        //  3. Write COMMIT record to WAL
        // )
        
        txn.active = false;
        txn.committed = true;
        
        return true;
    }
    
    // Rollback: play undo log in reverse
    bool rollback(int txn_id) {
        std::lock_guard<std::mutex> lock(txn_lock);
        
        if (!transactions.count(txn_id)) return false;
        Transaction& txn = transactions[txn_id];
        
        // Undo all changes in reverse order
        for (auto it = txn.undo_log.rbegin(); it != txn.undo_log.rend(); ++it) {
            // For INSERT undo: remove from local_writes
            if (it->operation == "INSERT") {
                std::string key = it->table_name + ":" + it->row_id;
                txn.local_writes.erase(key);
            }
            // For UPDATE undo: restore old version
            else if (it->operation == "UPDATE") {
                std::string key = it->table_name + ":" + it->row_id;
                txn.local_writes[key] = it->old_version;
            }
            // For DELETE undo: restore deleted row
            else if (it->operation == "DELETE") {
                std::string key = it->table_name + ":" + it->row_id;
                txn.local_writes[key] = it->old_version;
            }
        }
        
        txn.active = false;
        txn.committed = false;
        txn.local_writes.clear();
        txn.undo_log.clear();
        
        return true;
    }
};

}
