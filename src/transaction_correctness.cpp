#include <cstdint>
#include <string>
#include "dbx4/transaction_state.h"
#include <iostream>
#include <map>
#include <vector>

namespace dbx4 {

// Simulated database tables (simplified for testing)
static std::map<std::string, std::map<std::string, std::vector<VersionedRowWithTx>>> tables;
static std::map<int, Transaction> active_transactions;
static int next_tx_id = 1;

// P0-4 FIX: Implement COMMIT with atomic semantics
bool commit_transaction(int tx_id) {
    auto it = active_transactions.find(tx_id);
    if (it == active_transactions.end()) {
        std::cerr << "ERROR: Transaction " << tx_id << " not found" << std::endl;
        return false;
    }
    
    Transaction& tx = it->second;
    
    // Phase 1: Validation
    // Check all constraints for this transaction's writes
    for (const auto& [table_name, row_keys] : tx.write_set) {
        if (tables.find(table_name) == tables.end()) {
            std::cerr << "ERROR: Table " << table_name << " not found" << std::endl;
            return false;
        }
    }
    
    // Phase 2: Mark all versions as COMMITTED
    for (const auto& [table_name, row_keys] : tx.write_set) {
        auto& table = tables[table_name];
        for (const auto& row_key : row_keys) {
            if (table.find(row_key) != table.end()) {
                for (auto& version : table[row_key]) {
                    if (version.creator_tx_id == tx_id && version.tx_state == TxState::ACTIVE) {
                        version.tx_state = TxState::COMMITTED;
                    }
                }
            }
        }
    }
    
    // Phase 3: Log to WAL (would flush with fsync here)
    // log_manager.mark_committed(tx_id);
    // log_manager.flush_wal();
    
    tx.state = TxState::COMMITTED;
    active_transactions.erase(tx_id);
    
    std::cout << "✅ Transaction " << tx_id << " COMMITTED" << std::endl;
    return true;
}

// P0-4 FIX: Implement ROLLBACK to undo all changes
bool rollback_transaction(int tx_id) {
    auto it = active_transactions.find(tx_id);
    if (it == active_transactions.end()) {
        std::cerr << "ERROR: Transaction " << tx_id << " not found" << std::endl;
        return false;
    }
    
    Transaction& tx = it->second;
    
    // Mark all versions created by this transaction as ABORTED
    for (const auto& [table_name, row_keys] : tx.write_set) {
        if (tables.find(table_name) != tables.end()) {
            auto& table = tables[table_name];
            for (const auto& row_key : row_keys) {
                if (table.find(row_key) != table.end()) {
                    for (auto& version : table[row_key]) {
                        if (version.creator_tx_id == tx_id && version.tx_state == TxState::ACTIVE) {
                            version.tx_state = TxState::ABORTED;
                        }
                    }
                }
            }
        }
    }
    
    tx.state = TxState::ABORTED;
    active_transactions.erase(tx_id);
    
    std::cout << "✅ Transaction " << tx_id << " ROLLED BACK" << std::endl;
    return true;
}

// P0-4 FIX: Modified SELECT with visibility control
int count_visible_rows(int reader_tx_id, const std::string& table_name) {
    if (tables.find(table_name) == tables.end()) {
        return 0;
    }
    
    int count = 0;
    const auto& table = tables[table_name];
    
    for (const auto& [row_key, versions] : table) {
        // Get the latest version visible to this reader
        for (const auto& version : versions) {
            if (version.is_visible_to(reader_tx_id) && !version.is_deleted()) {
                count++;
                break;  // Only count the latest visible version per row
            }
        }
    }
    
    return count;
}

// BEGIN TRANSACTION
int begin_transaction() {
    int tx_id = next_tx_id++;
    Transaction tx;
    tx.tx_id = tx_id;
    tx.start_time = 0;  // Would be current timestamp
    tx.state = TxState::ACTIVE;
    
    active_transactions[tx_id] = tx;
    std::cout << "✅ Transaction " << tx_id << " STARTED" << std::endl;
    return tx_id;
}

// INSERT with transaction tracking
void insert_in_tx(int tx_id, const std::string& table_name, const std::string& row_key, 
                  const std::map<std::string, std::string>& data) {
    if (active_transactions.find(tx_id) == active_transactions.end()) {
        std::cerr << "ERROR: No active transaction " << tx_id << std::endl;
        return;
    }
    
    // Ensure table exists
    if (tables.find(table_name) == tables.end()) {
        tables[table_name] = {};
    }
    
    // Create new version
    VersionedRowWithTx version;
    version.data = data;
    version.version_id = 0;  // Simplified
    version.created_at = 0;
    version.deleted_at = -1;
    version.creator_tx_id = tx_id;  // Track which transaction created this
    version.tx_state = TxState::ACTIVE;  // Uncommitted initially
    
    tables[table_name][row_key].push_back(version);
    active_transactions[tx_id].write_set[table_name].insert(row_key);
    
    std::cout << "  INSERT: tx_id=" << tx_id << " table=" << table_name 
              << " row=" << row_key << " (ACTIVE)" << std::endl;
}

}  // namespace dbx4
