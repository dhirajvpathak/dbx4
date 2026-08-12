#ifndef DBX4_REPLICATION_MASTER_H
#define DBX4_REPLICATION_MASTER_H

#include <string>
#include <vector>
#include <map>
#include <iostream>

namespace dbx4 {

struct ReplicationSlot {
    std::string slot_name;
    std::string slot_type;
    long restart_lsn = 0;
    long confirmed_flush_lsn = 0;
    bool active = false;
};

class ReplicationMaster {
private:
    std::map<std::string, ReplicationSlot> slots_;
    std::vector<std::string> connected_replicas_;
    long current_lsn_ = 0;
    
public:
    ReplicationMaster() {}
    
    bool create_replication_slot(const std::string& slot_name, const std::string& slot_type) {
        ReplicationSlot slot;
        slot.slot_name = slot_name;
        slot.slot_type = slot_type;
        slot.restart_lsn = current_lsn_;
        slot.active = true;
        slots_[slot_name] = slot;
        std::cout << "[Master] Created slot: " << slot_name << " (" << slot_type << ")\n";
        return true;
    }
    
    bool drop_replication_slot(const std::string& slot_name) {
        slots_.erase(slot_name);
        std::cout << "[Master] Dropped slot: " << slot_name << "\n";
        return true;
    }
    
    bool register_replica(const std::string& replica_host, int replica_port) {
        connected_replicas_.push_back(replica_host + ":" + std::to_string(replica_port));
        std::cout << "[Master] Registered replica: " << replica_host << ":" << replica_port << "\n";
        return true;
    }
    
    void advance_lsn(long bytes) {
        current_lsn_ += bytes;
        std::cout << "[Master] Advanced LSN to: " << current_lsn_ << "\n";
    }
    
    long get_current_lsn() const {
        return current_lsn_;
    }
    
    int get_replica_count() const {
        return connected_replicas_.size();
    }
    
    int get_slot_count() const {
        return slots_.size();
    }
};

}

#endif
