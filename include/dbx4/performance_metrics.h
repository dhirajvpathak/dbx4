#ifndef DBX4_PERFORMANCE_METRICS_H
#define DBX4_PERFORMANCE_METRICS_H
#include <chrono>
#include <vector>
#include <iostream>
namespace dbx4 {
class PerformanceMetrics {
private:
    std::vector<double> operation_times_;
    std::chrono::high_resolution_clock::time_point start_;
public:
    void start_timer() {
        start_ = std::chrono::high_resolution_clock::now();
    }
    void end_timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(end - start_).count();
        operation_times_.push_back(duration);
    }
    double get_average_latency() {
        if (operation_times_.empty()) return 0;
        double sum = 0;
        for (auto t : operation_times_) sum += t;
        return sum / operation_times_.size();
    }
    double get_throughput(int operations) {
        double total_time = 0;
        for (auto t : operation_times_) total_time += t;
        return total_time > 0 ? operations / total_time : 0;
    }
};
}
#endif
