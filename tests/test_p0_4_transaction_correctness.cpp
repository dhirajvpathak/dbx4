#include <iostream>
#include <cassert>
#include <map>
#include <set>
#include <vector>
#include <string>

// Simplified Transaction State structures for testing
enum class TxState { ACTIVE, COMMITTED, ABORTED };

struct VersionedRowWithTx {
    std::map<std::string, std::string> data;
    int version_id = 0;
    int64_t created_at = 0;
    int64_t deleted_at = -1;
    int creator_tx_id = -1;
    TxState tx_state = TxState::ACTIVE;
    
    bool is_visible_to(int reader_tx_id) const {
        if (tx_state == TxState::COMMITTED) return true;
        if (creator_tx_id == reader_tx_id && tx_state == TxState::ACTIVE) return true;
        return false;
    }
    
    bool is_deleted() const { return deleted_at >= 0; }
};

struct Transaction {
    int tx_id = 0;
    TxState state = TxState::ACTIVE;
    std::map<std::string, std::set<std::string>> write_set;
};

// Global state
std::map<std::string, std::map<std::string, std::vector<VersionedRowWithTx>>> tables;
std::map<int, Transaction> active_transactions;
int next_tx_id = 1;

// Helper functions
int begin_tx() {
    int tx_id = next_tx_id++;
    active_transactions[tx_id] = {tx_id};
    return tx_id;
}

void insert_in_tx(int tx_id, const std::string& table, const std::string& key, 
                  const std::map<std::string, std::string>& data) {
    if (tables[table].find(key) == tables[table].end()) {
        tables[table][key] = {};
    }
    
    VersionedRowWithTx v;
    v.data = data;
    v.creator_tx_id = tx_id;
    v.tx_state = TxState::ACTIVE;
    tables[table][key].push_back(v);
    active_transactions[tx_id].write_set[table].insert(key);
}

bool commit_tx(int tx_id) {
    auto& tx = active_transactions[tx_id];
    for (const auto& [table, keys] : tx.write_set) {
        for (const auto& key : keys) {
            for (auto& v : tables[table][key]) {
                if (v.creator_tx_id == tx_id && v.tx_state == TxState::ACTIVE) {
                    v.tx_state = TxState::COMMITTED;
                }
            }
        }
    }
    tx.state = TxState::COMMITTED;
    return true;
}

bool rollback_tx(int tx_id) {
    auto& tx = active_transactions[tx_id];
    for (const auto& [table, keys] : tx.write_set) {
        for (const auto& key : keys) {
            for (auto& v : tables[table][key]) {
                if (v.creator_tx_id == tx_id && v.tx_state == TxState::ACTIVE) {
                    v.tx_state = TxState::ABORTED;
                }
            }
        }
    }
    tx.state = TxState::ABORTED;
    return true;
}

int count_visible(int reader_tx, const std::string& table) {
    if (tables.find(table) == tables.end()) return 0;
    int count = 0;
    for (const auto& [key, versions] : tables[table]) {
        for (const auto& v : versions) {
            if (v.is_visible_to(reader_tx) && !v.is_deleted()) {
                count++;
                break;
            }
        }
    }
    return count;
}

// P0-4 TEST 1: Uncommitted data hidden
void test_uncommitted_hidden() {
    std::cout << "TEST 1: Uncommitted data hidden from other transactions" << std::endl;
    tables.clear();
    active_transactions.clear();
    next_tx_id = 1;
    
    int tx_a = begin_tx();
    int tx_b = begin_tx();
    
    // Tx A inserts (uncommitted)
    insert_in_tx(tx_a, "users", "1", {{"name", "Alice"}});
    
    // Tx B tries to see it
    int visible_count = count_visible(tx_b, "users");
    assert(visible_count == 0 && "❌ FAILED: Uncommitted data visible!");
    
    // Commit Tx A
    commit_tx(tx_a);
    
    // Now Tx B should see it
    visible_count = count_visible(tx_b, "users");
    assert(visible_count == 1 && "❌ FAILED: Committed data not visible!");
    
    std::cout << "✅ PASSED: Uncommitted data correctly hidden" << std::endl << std::endl;
}

// P0-4 TEST 2: ROLLBACK removes changes
void test_rollback() {
    std::cout << "TEST 2: ROLLBACK removes changes" << std::endl;
    tables.clear();
    active_transactions.clear();
    next_tx_id = 1;
    
    int tx_a = begin_tx();
    int tx_b = begin_tx();
    
    // Tx A inserts
    insert_in_tx(tx_a, "users", "1", {{"name", "Bob"}});
    
    // Tx B doesn't see it
    assert(count_visible(tx_b, "users") == 0);
    
    // Tx A rolls back
    rollback_tx(tx_a);
    
    // Tx B still doesn't see it
    assert(count_visible(tx_b, "users") == 0 && "❌ FAILED: Rolled back data visible!");
    
    std::cout << "✅ PASSED: ROLLBACK correctly removes changes" << std::endl << std::endl;
}

// P0-4 TEST 3: Read-your-own-writes
void test_read_own_writes() {
    std::cout << "TEST 3: Read-your-own-writes" << std::endl;
    tables.clear();
    active_transactions.clear();
    next_tx_id = 1;
    
    int tx_a = begin_tx();
    int tx_b = begin_tx();
    
    // Tx A inserts
    insert_in_tx(tx_a, "users", "1", {{"name", "Charlie"}});
    
    // Tx A can see its own write
    int tx_a_sees = count_visible(tx_a, "users");
    assert(tx_a_sees == 1 && "❌ FAILED: Read-own-writes not working!");
    
    // Tx B cannot see it
    int tx_b_sees = count_visible(tx_b, "users");
    assert(tx_b_sees == 0 && "❌ FAILED: Other tx sees uncommitted data!");
    
    std::cout << "✅ PASSED: Read-your-own-writes works correctly" << std::endl << std::endl;
}

// P0-4 TEST 4: Atomic COMMIT
void test_atomic_commit() {
    std::cout << "TEST 4: Atomic COMMIT (all-or-nothing)" << std::endl;
    tables.clear();
    active_transactions.clear();
    next_tx_id = 1;
    
    int tx_a = begin_tx();
    
    // Multiple inserts in same transaction
    insert_in_tx(tx_a, "users", "1", {{"name", "Dave"}});
    insert_in_tx(tx_a, "users", "2", {{"name", "Eve"}});
    
    // Both uncommitted
    int tx_others = begin_tx();
    assert(count_visible(tx_others, "users") == 0);
    
    // Commit atomically
    bool success = commit_tx(tx_a);
    assert(success);
    
    // Now both visible
    assert(count_visible(tx_others, "users") == 2 && "❌ FAILED: Atomic commit failed!");
    
    std::cout << "✅ PASSED: Atomic COMMIT works correctly" << std::endl << std::endl;
}

// P0-4 TEST 5: Isolation level (READ COMMITTED)
void test_isolation_level() {
    std::cout << "TEST 5: Isolation level (READ COMMITTED)" << std::endl;
    tables.clear();
    active_transactions.clear();
    next_tx_id = 1;
    
    int tx_a = begin_tx();
    
    // Pre-insert some data
    insert_in_tx(tx_a, "users", "1", {{"name", "Initial"}});
    commit_tx(tx_a);
    
    // Tx A sees 1 row
    int count1 = count_visible(tx_a, "users");
    assert(count1 == 1);
    
    // Tx B inserts new row (as new transaction)
    int tx_c = begin_tx();
    insert_in_tx(tx_c, "users", "2", {{"name", "New"}});
    commit_tx(tx_c);
    
    // Tx A now sees 2 rows (phantom read allowed in READ COMMITTED)
    int count2 = count_visible(tx_a, "users");
    assert(count2 == 2 && "❌ FAILED: Isolation level enforcement failed!");
    
    std::cout << "✅ PASSED: Isolation level (READ COMMITTED) works" << std::endl << std::endl;
}

// Main test runner
int main() {
    std::cout << "=" << std::string(78, '=') << std::endl;
    std::cout << "P0-4 TRANSACTION CORRECTNESS TESTS" << std::endl;
    std::cout << "=" << std::string(78, '=') << std::endl << std::endl;
    
    try {
        test_uncommitted_hidden();
        test_rollback();
        test_read_own_writes();
        test_atomic_commit();
        test_isolation_level();
        
        std::cout << "=" << std::string(78, '=') << std::endl;
        std::cout << "✅ ALL P0-4 TESTS PASSED (5/5)" << std::endl;
        std::cout << "=" << std::string(78, '=') << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
}
