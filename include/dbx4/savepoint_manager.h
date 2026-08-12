#ifndef DBX4_SAVEPOINT_MANAGER_H
#define DBX4_SAVEPOINT_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <iostream>

namespace dbx4 {

struct Savepoint {
    std::string name;
    int transaction_id = 0;
    std::vector<std::string> operations;  // SQL statements
    long timestamp = 0;
};

class SavepointManager {
private:
    std::vector<Savepoint> savepoints_;
    int transaction_id_ = 0;
    
public:
    SavepointManager() {}
    
    bool begin_transaction() {
        transaction_id_++;
        std::cout << "[SavepointManager] BEGIN TRANSACTION " << transaction_id_ << "\n";
        return true;
    }
    
    bool create_savepoint(const std::string& name) {
        Savepoint sp;
        sp.name = name;
        sp.transaction_id = transaction_id_;
        sp.timestamp = savepoints_.size();
        savepoints_.push_back(sp);
        std::cout << "[SavepointManager] SAVEPOINT " << name << "\n";
        return true;
    }
    
    bool rollback_to_savepoint(const std::string& name) {
        for (int i = savepoints_.size() - 1; i >= 0; --i) {
            if (savepoints_[i].name == name && 
                savepoints_[i].transaction_id == transaction_id_) {
                // Remove all operations after this savepoint
                savepoints_.erase(savepoints_.begin() + i + 1, savepoints_.end());
                std::cout << "[SavepointManager] ROLLBACK TO SAVEPOINT " << name << "\n";
                return true;
            }
        }
        return false;
    }
    
    bool release_savepoint(const std::string& name) {
        for (size_t i = 0; i < savepoints_.size(); ++i) {
            if (savepoints_[i].name == name) {
                savepoints_.erase(savepoints_.begin() + i);
                std::cout << "[SavepointManager] RELEASE SAVEPOINT " << name << "\n";
                return true;
            }
        }
        return false;
    }
    
    bool add_operation(const std::string& sql) {
        if (!savepoints_.empty()) {
            savepoints_.back().operations.push_back(sql);
        }
        return true;
    }
    
    bool commit_transaction() {
        std::cout << "[SavepointManager] COMMIT TRANSACTION " << transaction_id_ << "\n";
        savepoints_.clear();
        return true;
    }
    
    bool rollback_transaction() {
        std::cout << "[SavepointManager] ROLLBACK TRANSACTION " << transaction_id_ << "\n";
        savepoints_.clear();
        return true;
    }
    
    int get_savepoint_count() const {
        return savepoints_.size();
    }
    
    std::string get_savepoint_name(int index) const {
        if (index < savepoints_.size()) {
            return savepoints_[index].name;
        }
        return "";
    }
};

}

#endif
