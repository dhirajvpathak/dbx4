#ifndef DBX4_SUBQUERY_ENGINE_H
#define DBX4_SUBQUERY_ENGINE_H

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace dbx4 {

struct CTE {
    std::string name;
    std::string query;
    std::vector<std::map<std::string, std::string>> result_set;
};

class SubqueryExecutor {
private:
    std::vector<CTE> ctes_;
    std::string main_query_;
    
public:
    SubqueryExecutor() {}
    
    bool add_cte(const std::string& name, const std::string& query) {
        CTE cte;
        cte.name = name;
        cte.query = query;
        ctes_.push_back(cte);
        return true;
    }
    
    bool set_main_query(const std::string& query) {
        main_query_ = query;
        return true;
    }
    
    std::string get_cte_name(size_t index) const {
        if (index < ctes_.size()) {
            return ctes_[index].name;
        }
        return "";
    }
    
    std::string get_cte_query(size_t index) const {
        if (index < ctes_.size()) {
            return ctes_[index].query;
        }
        return "";
    }
    
    // Scalar subquery: SELECT (SELECT COUNT(*) FROM table) AS count
    int execute_scalar_subquery(const std::string& subquery) {
        if (subquery.find("COUNT(*)") != std::string::npos) {
            return 10;  // Placeholder result
        }
        return 0;
    }
    
    // IN subquery: WHERE id IN (SELECT id FROM table)
    std::vector<int> execute_in_subquery(const std::string& subquery) {
        return {1, 2, 3, 4, 5};  // Placeholder results
    }
    
    // EXISTS subquery: WHERE EXISTS (SELECT 1 FROM table)
    bool execute_exists_subquery(const std::string& subquery) {
        return true;  // Placeholder
    }
    
    // Resolve CTE references
    bool resolve_cte(const std::string& cte_name, 
                     std::vector<std::map<std::string, std::string>>& result) {
        for (const auto& cte : ctes_) {
            if (cte.name == cte_name) {
                result = cte.result_set;
                return true;
            }
        }
        return false;
    }
    
    // Execute WITH clause
    bool execute_with_clause(const std::string& with_clause) {
        // Parse: WITH cte_name AS (query) SELECT ...
        if (with_clause.find("WITH") != std::string::npos) {
            std::cout << "[Subquery] WITH clause detected\n";
            return true;
        }
        return false;
    }
    
    size_t cte_count() const {
        return ctes_.size();
    }
    
    void print_ctes() {
        std::cout << "[Subquery] " << ctes_.size() << " CTEs defined\n";
        for (const auto& cte : ctes_) {
            std::cout << "  - " << cte.name << ": " << cte.query << "\n";
        }
    }
};

}

#endif
