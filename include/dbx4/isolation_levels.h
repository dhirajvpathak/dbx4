#ifndef DBX4_ISOLATION_LEVELS_H
#define DBX4_ISOLATION_LEVELS_H

#include <string>
#include <iostream>

namespace dbx4 {

enum class IsolationLevel {
    READ_UNCOMMITTED,
    READ_COMMITTED,
    REPEATABLE_READ,
    SERIALIZABLE
};

class IsolationManager {
private:
    IsolationLevel current_level_ = IsolationLevel::READ_COMMITTED;
    
public:
    IsolationManager() {}
    
    bool set_isolation_level(IsolationLevel level) {
        current_level_ = level;
        switch (level) {
            case IsolationLevel::READ_UNCOMMITTED:
                std::cout << "[Isolation] SET READ UNCOMMITTED (dirty reads allowed)\n";
                break;
            case IsolationLevel::READ_COMMITTED:
                std::cout << "[Isolation] SET READ COMMITTED (default)\n";
                break;
            case IsolationLevel::REPEATABLE_READ:
                std::cout << "[Isolation] SET REPEATABLE READ (no phantom reads)\n";
                break;
            case IsolationLevel::SERIALIZABLE:
                std::cout << "[Isolation] SET SERIALIZABLE (full isolation)\n";
                break;
        }
        return true;
    }
    
    bool allows_dirty_reads() const {
        return current_level_ == IsolationLevel::READ_UNCOMMITTED;
    }
    
    bool allows_phantom_reads() const {
        return current_level_ != IsolationLevel::SERIALIZABLE &&
               current_level_ != IsolationLevel::REPEATABLE_READ;
    }
    
    bool requires_shared_lock() const {
        return current_level_ == IsolationLevel::REPEATABLE_READ ||
               current_level_ == IsolationLevel::SERIALIZABLE;
    }
    
    bool requires_exclusive_lock() const {
        return current_level_ == IsolationLevel::SERIALIZABLE;
    }
    
    IsolationLevel get_current_level() const {
        return current_level_;
    }
    
    std::string get_level_name() const {
        switch (current_level_) {
            case IsolationLevel::READ_UNCOMMITTED:
                return "READ UNCOMMITTED";
            case IsolationLevel::READ_COMMITTED:
                return "READ COMMITTED";
            case IsolationLevel::REPEATABLE_READ:
                return "REPEATABLE READ";
            case IsolationLevel::SERIALIZABLE:
                return "SERIALIZABLE";
        }
        return "UNKNOWN";
    }
};

}

#endif
