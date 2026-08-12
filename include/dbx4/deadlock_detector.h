#ifndef DBX4_DEADLOCK_DETECTOR_H
#define DBX4_DEADLOCK_DETECTOR_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>

namespace dbx4 {

struct Transaction {
    int id = 0;
    std::vector<int> locked_resources;
    std::vector<int> waiting_for;
    long creation_time = 0;
};

class DeadlockDetector {
private:
    std::map<int, Transaction> transactions_;
    std::map<int, int> resource_owner_;  // resource_id -> transaction_id
    int next_tx_id_ = 1;
    
public:
    DeadlockDetector() {}
    
    int create_transaction() {
        Transaction tx;
        tx.id = next_tx_id_;
        tx.creation_time = next_tx_id_;
        transactions_[next_tx_id_] = tx;
        std::cout << "[DeadlockDetector] Created transaction " << next_tx_id_ << "\n";
        return next_tx_id_++;
    }
    
    bool acquire_lock(int tx_id, int resource_id) {
        if (resource_owner_.find(resource_id) == resource_owner_.end()) {
            resource_owner_[resource_id] = tx_id;
            transactions_[tx_id].locked_resources.push_back(resource_id);
            std::cout << "[DeadlockDetector] TX" << tx_id << " acquired lock on resource " << resource_id << "\n";
            return true;
        }
        
        int owner_id = resource_owner_[resource_id];
        if (owner_id != tx_id) {
            transactions_[tx_id].waiting_for.push_back(owner_id);
            std::cout << "[DeadlockDetector] TX" << tx_id << " waiting for TX" << owner_id << "\n";
            
            if (has_cycle(tx_id, owner_id)) {
                std::cout << "[DeadlockDetector] DEADLOCK DETECTED! Aborting TX" << tx_id << "\n";
                return false;
            }
        }
        return true;
    }
    
    bool release_lock(int tx_id, int resource_id) {
        resource_owner_.erase(resource_id);
        auto& locked = transactions_[tx_id].locked_resources;
        locked.erase(std::remove(locked.begin(), locked.end(), resource_id), locked.end());
        std::cout << "[DeadlockDetector] TX" << tx_id << " released lock on resource " << resource_id << "\n";
        return true;
    }
    
    bool has_cycle(int start_tx, int current_tx, std::set<int> visited = {}) {
        if (visited.find(current_tx) != visited.end()) {
            return true;  // Cycle detected
        }
        
        visited.insert(current_tx);
        
        if (transactions_.find(current_tx) != transactions_.end()) {
            for (int waiting_for_tx : transactions_[current_tx].waiting_for) {
                if (waiting_for_tx == start_tx) {
                    return true;  // Cycle back to start
                }
                if (has_cycle(start_tx, waiting_for_tx, visited)) {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    void abort_transaction(int tx_id) {
        if (transactions_.find(tx_id) != transactions_.end()) {
            // Release all locks
            for (int resource_id : transactions_[tx_id].locked_resources) {
                resource_owner_.erase(resource_id);
            }
            transactions_.erase(tx_id);
            std::cout << "[DeadlockDetector] Transaction " << tx_id << " aborted\n";
        }
    }
    
    int get_transaction_count() const {
        return transactions_.size();
    }
};

}

#endif
