#ifndef DBX4_FUNCTION_ENGINE_H
#define DBX4_FUNCTION_ENGINE_H

#include <string>
#include <vector>
#include <map>
#include <iostream>

namespace dbx4 {

struct FunctionParameter {
    std::string name;
    std::string type;
};

class FunctionEngine {
private:
    std::map<std::string, std::string> function_bodies_;
    std::map<std::string, std::vector<FunctionParameter>> function_params_;
    
public:
    FunctionEngine() {}
    
    bool create_function(const std::string& name, 
                        const std::vector<FunctionParameter>& params,
                        const std::string& return_type,
                        const std::string& body) {
        function_params_[name] = params;
        function_bodies_[name] = body;
        std::cout << "[FunctionEngine] Created function: " << name << "\n";
        return true;
    }
    
    bool function_exists(const std::string& name) const {
        return function_bodies_.find(name) != function_bodies_.end();
    }
    
    std::string invoke_function(const std::string& name, const std::vector<std::string>& args) {
        if (!function_exists(name)) {
            std::cout << "[FunctionEngine] Function not found: " << name << "\n";
            return "";
        }
        
        // Simple calculation for demonstrate
        if (name == "calculate_discount" && args.size() == 2) {
            try {
                double price = std::stod(args[0]);
                double discount = std::stod(args[1]);
                double result = price * (1 - discount / 100);
                std::cout << "[FunctionEngine] calculate_discount(" << price << ", " << discount << ") = " << result << "\n";
                return std::to_string(result);
            } catch (...) {
                return "";
            }
        }
        
        std::cout << "[FunctionEngine] Invoked function: " << name << "\n";
        return "OK";
    }
    
    bool drop_function(const std::string& name) {
        function_bodies_.erase(name);
        function_params_.erase(name);
        std::cout << "[FunctionEngine] Dropped function: " << name << "\n";
        return true;
    }
    
    int get_function_count() const {
        return function_bodies_.size();
    }
};

}

#endif
