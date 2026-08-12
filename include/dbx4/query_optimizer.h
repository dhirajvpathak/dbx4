#ifndef DBX4_QUERY_OPTIMIZER_H
#define DBX4_QUERY_OPTIMIZER_H

#include <string>
#include <vector>
#include <map>
#include <iostream>

namespace dbx4 {

struct IndexInfo {
    std::string index_name;
    std::string table_name;
    std::vector<std::string> columns;
    int selectivity = 100;  // percentage
    bool is_hash = false;
    bool is_btree = false;
};

struct QueryPlan {
    std::string operation;
    std::string index_used;
    int estimated_cost = 0;
    int estimated_rows = 0;
    std::string join_strategy;
};

class QueryOptimizer {
private:
    std::vector<IndexInfo> available_indexes_;
    std::map<std::string, int> table_row_counts_;
    
public:
    QueryOptimizer() {}
    
    void register_index(const IndexInfo& idx) {
        available_indexes_.push_back(idx);
    }
    
    void register_table(const std::string& table_name, int row_count) {
        table_row_counts_[table_name] = row_count;
    }
    
    // Find best index for a query
    IndexInfo find_best_index(const std::string& table, const std::string& column) {
        IndexInfo best;
        int best_selectivity = 0;
        
        for (const auto& idx : available_indexes_) {
            if (idx.table_name == table) {
                // Check if index contains the column
                for (const auto& col : idx.columns) {
                    if (col == column && idx.selectivity > best_selectivity) {
                        best = idx;
                        best_selectivity = idx.selectivity;
                    }
                }
            }
        }
        
        return best;
    }
    
    // Calculate query cost
    int calculate_cost(const std::string& operation, bool use_index) {
        if (use_index) {
            return 10;  // Index scan cost
        } else {
            return 100;  // Full table scan cost
        }
    }
    
    // Optimize WHERE clause
    QueryPlan optimize_where(const std::string& table, const std::string& condition) {
        QueryPlan plan;
        plan.operation = "Scan";
        
        // Check if we can use an index
        if (condition.find("=") != std::string::npos) {
            // Equality condition - can use hash or btree index
            plan.index_used = "hash_index";
            plan.estimated_cost = 10;
            plan.estimated_rows = 1;
        } else {
            // Range condition - better for btree
            plan.index_used = "btree_index";
            plan.estimated_cost = 15;
            plan.estimated_rows = 10;
        }
        
        return plan;
    }
    
    // Optimize JOIN order
    QueryPlan optimize_join(const std::string& left_table, const std::string& right_table) {
        QueryPlan plan;
        
        int left_rows = table_row_counts_[left_table];
        int right_rows = table_row_counts_[right_table];
        
        // Join smaller table first
        if (left_rows <= right_rows) {
            plan.operation = "HashJoin";
            plan.join_strategy = left_table + " -> " + right_table;
        } else {
            plan.operation = "HashJoin";
            plan.join_strategy = right_table + " -> " + left_table;
        }
        
        plan.estimated_cost = left_rows + right_rows;
        plan.estimated_rows = std::min(left_rows, right_rows);
        
        return plan;
    }
    
    // Predicate pushdown
    QueryPlan optimize_predicate_pushdown(const std::string& join_condition, 
                                          const std::string& where_clause) {
        QueryPlan plan;
        plan.operation = "Filter before join";
        plan.estimated_cost = 5;  // Cheaper than post-join filter
        return plan;
    }
    
    // Generate full query plan
    QueryPlan generate_plan(const std::string& query) {
        QueryPlan plan;
        
        if (query.find("JOIN") != std::string::npos) {
            plan.operation = "HashJoin with predicate pushdown";
            plan.estimated_cost = 20;
        } else if (query.find("WHERE") != std::string::npos) {
            plan.operation = "Index scan + Filter";
            plan.estimated_cost = 10;
        } else {
            plan.operation = "Full table scan";
            plan.estimated_cost = 100;
        }
        
        return plan;
    }
    
    void print_plan(const QueryPlan& plan) {
        std::cout << "[Optimizer] Plan:\n";
        std::cout << "  Operation: " << plan.operation << "\n";
        if (!plan.index_used.empty()) {
            std::cout << "  Index: " << plan.index_used << "\n";
        }
        std::cout << "  Estimated cost: " << plan.estimated_cost << "\n";
        std::cout << "  Estimated rows: " << plan.estimated_rows << "\n";
    }
};

}

#endif
