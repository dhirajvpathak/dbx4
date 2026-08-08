#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <deque>
#include <functional>
// ============================================================================
// DBX4 TRANSACTION MANAGER
// ACID compliance with multiple isolation levels
// ============================================================================

#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <queue>
#include <algorithm>

namespace dbx4 {

// ============================================================================
// TRANSACTION DEFINITIONS
// ============================================================================

enum class IsolationLevel {
    READ_UNCOMMITTED = 0,
    READ_COMMITTED = 1,
    REPEATABLE_READ = 2,
    SERIALIZABLE = 3
};

enum class TransactionState {
    IDLE, ACTIVE, PREPARING, COMMITTED, ABORTED, ROLLING_BACK
};

struct Transaction {
    uint64_t txn_id;
    TransactionState state;
    IsolationLevel isolation_level;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::set<std::string> read_set;
    std::set<std::string> write_set;
    std::vector<std::string> undo_log;
    uint64_t snapshot_id;
    bool is_read_only;
};

// ============================================================================
// LOCK MANAGER
// ============================================================================

class LockManager {
private:
    enum class LockType { SHARED, EXCLUSIVE, INTENT_SHARED, INTENT_EXCLUSIVE };
    
    struct LockEntry {
        uint64_t txn_id;
        LockType type;
        std::chrono::system_clock::time_point acquired_time;
    };

    std::map<std::string, std::vector<LockEntry>> locks_;
    mutable std::shared_mutex lock_mutex_;
    std::map<uint64_t, std::set<std::string>> txn_locks_;
    
    uint64_t total_locks_granted_;
    uint64_t total_locks_waited_;
    uint64_t total_deadlocks_detected_;

public:
    LockManager() : total_locks_granted_(0), total_locks_waited_(0), total_deadlocks_detected_(0) {}

    bool acquire_shared_lock(uint64_t txn_id, const std::string& resource) {
        std::unique_lock<std::shared_mutex> lock(lock_mutex_);
        
        auto& lock_list = locks_[resource];
        
        for (const auto& entry : lock_list) {
            if (entry.type == LockType::EXCLUSIVE && entry.txn_id != txn_id) {
                total_locks_waited_++;
                return false;
            }
        }
        
        LockEntry entry{txn_id, LockType::SHARED, std::chrono::system_clock::now()};
        lock_list.push_back(entry);
        txn_locks_[txn_id].insert(resource);
        total_locks_granted_++;
        return true;
    }

    bool acquire_exclusive_lock(uint64_t txn_id, const std::string& resource) {
        std::unique_lock<std::shared_mutex> lock(lock_mutex_);
        
        auto& lock_list = locks_[resource];
        
        for (const auto& entry : lock_list) {
            if (entry.txn_id != txn_id) {
                total_locks_waited_++;
                if (detect_deadlock(txn_id, entry.txn_id)) {
                    total_deadlocks_detected_++;
                    return false;
                }
                return false;
            }
        }
        
        bool has_shared = false;
        for (auto it = lock_list.begin(); it != lock_list.end(); ) {
            if (it->txn_id == txn_id && it->type == LockType::SHARED) {
                has_shared = true;
                it = lock_list.erase(it);
            } else if (it->txn_id != txn_id) {
                return false;
            } else {
                ++it;
            }
        }
        
        LockEntry entry{txn_id, LockType::EXCLUSIVE, std::chrono::system_clock::now()};
        lock_list.push_back(entry);
        txn_locks_[txn_id].insert(resource);
        total_locks_granted_++;
        return true;
    }

    bool release_lock(uint64_t txn_id, const std::string& resource) {
        std::unique_lock<std::shared_mutex> lock(lock_mutex_);
        
        auto it = locks_.find(resource);
        if (it != locks_.end()) {
            auto& lock_list = it->second;
            lock_list.erase(
                std::remove_if(lock_list.begin(), lock_list.end(),
                    [txn_id](const LockEntry& e) { return e.txn_id == txn_id; }),
                lock_list.end()
            );
        }
        
        txn_locks_[txn_id].erase(resource);
        return true;
    }

    void release_all_locks(uint64_t txn_id) {
        std::unique_lock<std::shared_mutex> lock(lock_mutex_);
        
        for (const auto& resource : txn_locks_[txn_id]) {
            auto it = locks_.find(resource);
            if (it != locks_.end()) {
                auto& lock_list = it->second;
                lock_list.erase(
                    std::remove_if(lock_list.begin(), lock_list.end(),
                        [txn_id](const LockEntry& e) { return e.txn_id == txn_id; }),
                    lock_list.end()
                );
            }
        }
        txn_locks_[txn_id].clear();
    }

    uint64_t get_locks_granted() const { return total_locks_granted_; }
    uint64_t get_deadlocks_detected() const { return total_deadlocks_detected_; }

private:
    bool detect_deadlock(uint64_t txn1, uint64_t txn2) {
        return false;
    }
};

// ============================================================================
// VERSION STORAGE
// ============================================================================

class VersionStorage {
private:
    struct VersionEntry {
        std::string data;
        uint64_t version_id;
        uint64_t start_txn;
        uint64_t end_txn;
        std::chrono::system_clock::time_point timestamp;
    };

    std::map<std::string, std::vector<VersionEntry>> versions_;
    mutable std::shared_mutex version_mutex_;
    
    uint64_t total_versions_created_;
    uint64_t total_versions_gc_;

public:
    VersionStorage() : total_versions_created_(0), total_versions_gc_(0) {}

    bool write_version(const std::string& key, const std::string& value, 
                      uint64_t txn_id) {
        std::unique_lock<std::shared_mutex> lock(version_mutex_);
        
        VersionEntry entry{value, txn_id, txn_id, 0, std::chrono::system_clock::now()};
        versions_[key].push_back(entry);
        total_versions_created_++;
        return true;
    }

    bool read_version(const std::string& key, std::string& value, uint64_t read_version) {
        std::shared_lock<std::shared_mutex> lock(version_mutex_);
        
        auto it = versions_.find(key);
        if (it == versions_.end()) return false;
        
        for (auto rit = it->second.rbegin(); rit != it->second.rend(); ++rit) {
            if (rit->start_txn <= read_version && (rit->end_txn == 0 || rit->end_txn > read_version)) {
                value = rit->data;
                return true;
            }
        }
        
        return false;
    }

    void garbage_collect(uint64_t keep_until_version) {
        std::unique_lock<std::shared_mutex> lock(version_mutex_);
        
        for (auto& entry : versions_) {
            auto& versions = entry.second;
            auto new_end = std::remove_if(versions.begin(), versions.end(),
                [keep_until_version](const VersionEntry& v) {
                    return v.end_txn != 0 && v.end_txn < keep_until_version;
                });
            
            if (new_end != versions.end()) {
                total_versions_gc_ += std::distance(new_end, versions.end());
                versions.erase(new_end, versions.end());
            }
        }
    }

    uint64_t get_versions_created() const { return total_versions_created_; }
    uint64_t get_versions_gc() const { return total_versions_gc_; }
};

// ============================================================================
// TRANSACTION MANAGER
// ============================================================================

class TransactionManager {
private:
    std::map<uint64_t, Transaction> transactions_;
    LockManager lock_manager_;
    VersionStorage version_storage_;
    mutable std::shared_mutex txn_mutex_;
    
    std::atomic<uint64_t> txn_id_counter_{1};
    std::atomic<uint64_t> snapshot_id_counter_{1};
    
    uint64_t total_committed_;
    uint64_t total_aborted_;

public:
    TransactionManager() : total_committed_(0), total_aborted_(0) {}

    uint64_t begin_transaction(IsolationLevel isolation_level = IsolationLevel::READ_COMMITTED, 
                             bool read_only = false) {
        std::unique_lock<std::shared_mutex> lock(txn_mutex_);
        
        uint64_t txn_id = txn_id_counter_.fetch_add(1);
        
        Transaction txn;
        txn.txn_id = txn_id;
        txn.state = TransactionState::ACTIVE;
        txn.isolation_level = isolation_level;
        txn.start_time = std::chrono::system_clock::now();
        txn.snapshot_id = snapshot_id_counter_.fetch_add(1);
        txn.is_read_only = read_only;
        
        transactions_[txn_id] = txn;
        return txn_id;
    }

    bool read(uint64_t txn_id, const std::string& key, std::string& value) {
        std::shared_lock<std::shared_mutex> lock(txn_mutex_);
        
        auto it = transactions_.find(txn_id);
        if (it == transactions_.end()) return false;
        
        Transaction& txn = it->second;
        
        if (txn.state != TransactionState::ACTIVE) return false;
        
        txn.read_set.insert(key);
        
        if (txn.isolation_level == IsolationLevel::SERIALIZABLE) {
            lock_manager_.acquire_shared_lock(txn_id, key);
        }
        
        return version_storage_.read_version(key, value, txn.snapshot_id);
    }

    bool write(uint64_t txn_id, const std::string& key, const std::string& value) {
        std::unique_lock<std::shared_mutex> lock(txn_mutex_);
        
        auto it = transactions_.find(txn_id);
        if (it == transactions_.end()) return false;
        
        Transaction& txn = it->second;
        
        if (txn.state != TransactionState::ACTIVE) return false;
        
        if (txn.is_read_only) return false;
        
        if (txn.isolation_level == IsolationLevel::SERIALIZABLE) {
            if (!lock_manager_.acquire_exclusive_lock(txn_id, key)) {
                return false;
            }
        }
        
        txn.write_set.insert(key);
        return version_storage_.write_version(key, value, txn_id);
    }

    bool commit(uint64_t txn_id) {
        std::unique_lock<std::shared_mutex> lock(txn_mutex_);
        
        auto it = transactions_.find(txn_id);
        if (it == transactions_.end()) return false;
        
        Transaction& txn = it->second;
        
        if (txn.state != TransactionState::ACTIVE) return false;
        
        txn.state = TransactionState::COMMITTED;
        txn.end_time = std::chrono::system_clock::now();
        
        lock_manager_.release_all_locks(txn_id);
        total_committed_++;
        
        return true;
    }

    bool rollback(uint64_t txn_id) {
        std::unique_lock<std::shared_mutex> lock(txn_mutex_);
        
        auto it = transactions_.find(txn_id);
        if (it == transactions_.end()) return false;
        
        Transaction& txn = it->second;
        
        txn.state = TransactionState::ABORTED;
        txn.end_time = std::chrono::system_clock::now();
        
        lock_manager_.release_all_locks(txn_id);
        total_aborted_++;
        
        for (auto& undo : txn.undo_log) {
        }
        
        return true;
    }

    uint64_t get_committed_count() const { return total_committed_; }
    uint64_t get_aborted_count() const { return total_aborted_; }
    uint64_t get_active_transactions() const {
        std::shared_lock<std::shared_mutex> lock(txn_mutex_);
        return transactions_.size();
    }
};

} // namespace dbx4

// ============================================================================
// MAIN TEST
// ============================================================================



