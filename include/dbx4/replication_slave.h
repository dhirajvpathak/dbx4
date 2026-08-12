#ifndef DBX4_REPLICATION_SLAVE_H
#define DBX4_REPLICATION_SLAVE_H

#include <string>
#include <iostream>

namespace dbx4 {

class ReplicationSlave {
private:
    std::string master_host_;
    int master_port_ = 0;
    std::string replication_user_;
    long replica_lsn_ = 0;
    long replication_lag_ms_ = 0;
    bool connected_ = false;
    
public:
    ReplicationSlave() {}
    
    bool connect_to_master(const std::string& master_host, int master_port,
                          const std::string& user, const std::string& password) {
        master_host_ = master_host;
        master_port_ = master_port;
        replication_user_ = user;
        connected_ = true;
        std::cout << "[Slave] Connected to master: " << master_host << ":" << master_port << "\n";
        return true;
    }
    
    bool start_replication(const std::string& slot_name) {
        std::cout << "[Slave] Starting replication from slot: " << slot_name << "\n";
        return true;
    }
    
    void apply_wal_record(const std::string& wal_data) {
        replica_lsn_++;
        std::cout << "[Slave] Applied WAL record, LSN: " << replica_lsn_ << "\n";
    }
    
    long get_replica_lsn() const {
        return replica_lsn_;
    }
    
    long calculate_replication_lag(long master_lsn) {
        replication_lag_ms_ = (master_lsn - replica_lsn_) * 10;
        std::cout << "[Slave] Replication lag: " << replication_lag_ms_ << " ms\n";
        return replication_lag_ms_;
    }
    
    bool is_connected() const {
        return connected_;
    }
    
    bool disconnect() {
        connected_ = false;
        std::cout << "[Slave] Disconnected from master\n";
        return true;
    }
};

}

#endif
