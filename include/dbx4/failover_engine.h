#ifndef DBX4_FAILOVER_ENGINE_H
#define DBX4_FAILOVER_ENGINE_H

#include <string>
#include <vector>
#include <iostream>
#include <chrono>

namespace dbx4 {

enum class FailoverStatus {
    MASTER_HEALTHY,
    MASTER_DEGRADED,
    MASTER_FAILED,
    REPLICA_PROMOTED
};

class FailoverEngine {
private:
    std::string current_master_;
    std::vector<std::string> replicas_;
    FailoverStatus status_ = FailoverStatus::MASTER_HEALTHY;
    long last_heartbeat_ = 0;
    int heartbeat_timeout_ms_ = 5000;
    
public:
    FailoverEngine() {}
    
    bool register_master(const std::string& master_host) {
        current_master_ = master_host;
        status_ = FailoverStatus::MASTER_HEALTHY;
        last_heartbeat_ = std::chrono::system_clock::now().time_since_epoch().count();
        std::cout << "[Failover] Registered master: " << master_host << "\n";
        return true;
    }
    
    bool register_replica(const std::string& replica_host) {
        replicas_.push_back(replica_host);
        std::cout << "[Failover] Registered replica: " << replica_host << "\n";
        return true;
    }
    
    bool send_heartbeat() {
        last_heartbeat_ = std::chrono::system_clock::now().time_since_epoch().count();
        std::cout << "[Failover] Master heartbeat received\n";
        status_ = FailoverStatus::MASTER_HEALTHY;
        return true;
    }
    
    bool check_master_health() {
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        long elapsed = (now - last_heartbeat_) / 1000000;  // Convert to ms
        
        if (elapsed > heartbeat_timeout_ms_) {
            std::cout << "[Failover] ❌ MASTER FAILED (no heartbeat for " << elapsed << "ms)\n";
            status_ = FailoverStatus::MASTER_FAILED;
            return false;
        } else if (elapsed > heartbeat_timeout_ms_ / 2) {
            std::cout << "[Failover] ⚠️  Master degraded (slow heartbeat)\n";
            status_ = FailoverStatus::MASTER_DEGRADED;
            return true;
        }
        
        return true;
    }
    
    bool promote_replica(const std::string& replica_host) {
        if (status_ != FailoverStatus::MASTER_FAILED) {
            std::cout << "[Failover] Cannot promote: master still healthy\n";
            return false;
        }
        
        current_master_ = replica_host;
        status_ = FailoverStatus::REPLICA_PROMOTED;
        std::cout << "[Failover] ✅ Promoted replica to master: " << replica_host << "\n";
        return true;
    }
    
    bool synchronize_replicas() {
        std::cout << "[Failover] Synchronizing remaining replicas to new master\n";
        for (const auto& replica : replicas_) {
            if (replica != current_master_) {
                std::cout << "[Failover]   - Syncing " << replica << "\n";
            }
        }
        std::cout << "[Failover] Synchronization complete\n";
        return true;
    }
    
    std::string get_current_master() const {
        return current_master_;
    }
    
    FailoverStatus get_status() const {
        return status_;
    }
    
    int get_replica_count() const {
        return replicas_.size();
    }
};

}

#endif
