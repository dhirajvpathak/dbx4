#ifndef DBX4_TRANSACTION_STATE_H
#define DBX4_TRANSACTION_STATE_H

#include <cstdint>
#include <map>
#include <set>
#include <vector>

namespace dbx4 {

// Transaction state enumeration
enum class TxState {
    ACTIVE,      // Transaction in progress
    COMMITTED,   // Transaction committed (durable)
    ABORTED      // Transaction rolled back
};

// Updated VersionedRow with transaction visibility support
struct VersionedRowWithTx {
    std::map<std::string, std::string> data;
    int version_id = 0;
    int64_t created_at = 0;
    int64_t deleted_at = -1;
    int creator_tx_id = -1;  // NEW: Which transaction created this version
    TxState tx_state = TxState::ACTIVE;  // NEW: Transaction state
    
    // Check if this version is visible to a reader
    bool is_visible_to(int reader_tx_id) const {
        // Visible if:
        // 1. Committed versions visible to all, OR
        // 2. Reader can see their own uncommitted writes
        if (tx_state == TxState::COMMITTED) {
            return true;
        }
        if (creator_tx_id == reader_tx_id && tx_state == TxState::ACTIVE) {
            return true;  // Read-your-own-writes
        }
        return false;
    }
    
    bool is_deleted() const {
        return deleted_at >= 0;
    }
};

// Transaction context
struct Transaction {
    int tx_id = 0;
    int64_t start_time = 0;
    TxState state = TxState::ACTIVE;
    std::map<std::string, std::set<std::string>> write_set;  // table_name -> {row_keys}
    std::map<std::string, std::set<std::string>> read_set;   // table_name -> {row_keys}
};

}  // namespace dbx4

#endif  // DBX4_TRANSACTION_STATE_H
