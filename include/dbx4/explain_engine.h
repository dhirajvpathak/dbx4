#ifndef DBX4_EXPLAIN_ENGINE_H
#define DBX4_EXPLAIN_ENGINE_H

#include <string>
#include <vector>
#include <iostream>
#include <chrono>

namespace dbx4 {

struct ExplainOutput {
    std::string operation;
    std::string table_or_index;
    int cost = 0;
    int rows = 0;
    std::string filter;
    std::string index_name;
};

class ExplainExecutor {
private:
    bool analyze_ = false;
    std::vector<ExplainOutput> plan_steps_;
    
public:
    ExplainExecutor() {}
    
    void set_analyze(bool analyze) {
        analyze_ = analyze;
    }
    
    void add_plan_step(const ExplainOutput& step) {
        plan_steps_.push_back(step);
    }
    
    void explain_seq_scan(const std::string& table, int rows) {
        ExplainOutput step;
        step.operation = "Seq Scan";
        step.table_or_index = table;
        step.cost = rows / 10;
        step.rows = rows;
        plan_steps_.push_back(step);
    }
    
    void explain_index_scan(const std::string& index, const std::string& table, int rows) {
        ExplainOutput step;
        step.operation = "Index Scan";
        step.table_or_index = table;
        step.index_name = index;
        step.cost = 10;  // Much cheaper
        step.rows = rows;
        plan_steps_.push_back(step);
    }
    
    void explain_filter(const std::string& condition, int rows_before, int rows_after) {
        ExplainOutput step;
        step.operation = "Filter";
        step.filter = condition;
        step.rows = rows_after;
        step.cost = 5;
        plan_steps_.push_back(step);
    }
    
    void explain_hash_join(const std::string& left, const std::string& right, int rows) {
        ExplainOutput step;
        step.operation = "Hash Join";
        step.table_or_index = left + " <-> " + right;
        step.cost = 20;
        step.rows = rows;
        plan_steps_.push_back(step);
    }
    
    void explain_aggregate(const std::string& agg_func, int rows) {
        ExplainOutput step;
        step.operation = "Aggregate";
        step.filter = agg_func;
        step.cost = 15;
        step.rows = rows;
        plan_steps_.push_back(step);
    }
    
    void print_plan() {
        std::cout << "EXPLAIN PLAN:\n";
        for (size_t i = 0; i < plan_steps_.size(); ++i) {
            const auto& step = plan_steps_[i];
            std::cout << "  " << (i + 1) << ". " << step.operation;
            
            if (!step.table_or_index.empty()) {
                std::cout << " on " << step.table_or_index;
            }
            
            if (!step.index_name.empty()) {
                std::cout << " using " << step.index_name;
            }
            
            std::cout << "\n";
            std::cout << "     Cost: " << step.cost << ", Rows: " << step.rows;
            
            if (!step.filter.empty()) {
                std::cout << ", Filter: " << step.filter;
            }
            
            std::cout << "\n";
        }
    }
    
    void print_analyze(long long planning_ms, long long execution_ms) {
        std::cout << "\nANALYZE:\n";
        std::cout << "  Planning time: " << planning_ms << " ms\n";
        std::cout << "  Execution time: " << execution_ms << " ms\n";
    }
    
    int total_cost() const {
        int total = 0;
        for (const auto& step : plan_steps_) {
            total += step.cost;
        }
        return total;
    }
    
    int total_rows() const {
        if (!plan_steps_.empty()) {
            return plan_steps_.back().rows;
        }
        return 0;
    }
};

}

#endif
