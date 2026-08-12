#ifndef DBX4_MONITORING_H
#define DBX4_MONITORING_H
#include <string>
#include <map>
#include <iostream>
namespace dbx4 {
class MonitoringAgent {
private:
    std::map<std::string, double> metrics_;
public:
    void record_metric(const std::string& name, double value) {
        metrics_[name] = value;
        std::cout << "[Monitor] " << name << " = " << value << "\n";
    }
    double get_metric(const std::string& name) {
        return metrics_[name];
    }
    void report_health() {
        std::cout << "[Monitor] Health Check:\n";
        for (auto& [name, value] : metrics_) {
            std::cout << "  " << name << ": " << value << "\n";
        }
    }
};
}
#endif
