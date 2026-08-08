#ifndef DBX4_MONITORING_H
#define DBX4_MONITORING_H

#include <string>
#include <chrono>
#include <cstdint>
#include <vector>
#include <map>

namespace dbx4 {

struct MetricSnapshot {
    int64_t timestamp;
    double insert_rate;      // ops/sec
    double select_rate;      // ops/sec
    double update_rate;      // ops/sec
    double delete_rate;      // ops/sec
    int64_t active_transactions;
    int64_t memory_used_mb;
    double p99_latency_ms;
    int64_t errors_count;
};

class OperationalMonitoring {
private:
    std::vector<MetricSnapshot> snapshots;
    int64_t start_time;
    
public:
    OperationalMonitoring() {
        start_time = std::chrono::system_clock::now().time_since_epoch().count();
    }
    
    void record_snapshot(const MetricSnapshot& snap) {
        snapshots.push_back(snap);
    }
    
    bool check_health() {
        if (snapshots.empty()) return true;
        
        const auto& latest = snapshots.back();
        
        // Health checks
        bool healthy = true;
        
        if (latest.p99_latency_ms > 100) {
            healthy = false;  // P99 latency too high
        }
        
        if (latest.errors_count > 100) {
            healthy = false;  // Too many errors
        }
        
        if (latest.memory_used_mb > 10000) {
            healthy = false;  // Memory usage too high
        }
        
        return healthy;
    }
    
    void generate_report() {
        if (snapshots.empty()) return;
        
        std::cout << "OPERATIONAL HEALTH REPORT\n";
        std::cout << "=========================\n";
        
        const auto& latest = snapshots.back();
        
        std::cout << "Insert Rate: " << latest.insert_rate << " ops/sec\n";
        std::cout << "Select Rate: " << latest.select_rate << " ops/sec\n";
        std::cout << "Update Rate: " << latest.update_rate << " ops/sec\n";
        std::cout << "Delete Rate: " << latest.delete_rate << " ops/sec\n";
        std::cout << "Active Txns: " << latest.active_transactions << "\n";
        std::cout << "Memory Used: " << latest.memory_used_mb << " MB\n";
        std::cout << "P99 Latency: " << latest.p99_latency_ms << " ms\n";
        std::cout << "Error Count: " << latest.errors_count << "\n";
        std::cout << "Health: " << (check_health() ? "✅ HEALTHY" : "❌ DEGRADED") << "\n";
    }
};

}  // namespace dbx4

#endif
