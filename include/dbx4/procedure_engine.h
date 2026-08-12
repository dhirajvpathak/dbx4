#ifndef DBX4_PROCEDURE_ENGINE_H
#define DBX4_PROCEDURE_ENGINE_H

#include <string>
#include <vector>
#include <map>
#include <iostream>

namespace dbx4 {

struct ProcedureParameter {
    std::string name;
    std::string type;
    std::string direction;  // IN, OUT, INOUT
};

class ProcedureEngine {
private:
    std::map<std::string, std::string> procedure_bodies_;
    std::map<std::string, std::vector<ProcedureParameter>> procedure_params_;
    
public:
    ProcedureEngine() {}
    
    bool create_procedure(const std::string& name,
                         const std::vector<ProcedureParameter>& params,
                         const std::string& body) {
        procedure_params_[name] = params;
        procedure_bodies_[name] = body;
        std::cout << "[ProcedureEngine] Created procedure: " << name << "\n";
        return true;
    }
    
    bool procedure_exists(const std::string& name) const {
        return procedure_bodies_.find(name) != procedure_bodies_.end();
    }
    
    bool execute_procedure(const std::string& name, const std::vector<std::string>& args) {
        if (!procedure_exists(name)) {
            std::cout << "[ProcedureEngine] Procedure not found: " << name << "\n";
            return false;
        }
        
        // Simple DML simulation
        if (name == "transfer_funds" && args.size() == 3) {
            std::cout << "[ProcedureEngine] BEGIN TRANSACTION\n";
            std::cout << "[ProcedureEngine] UPDATE accounts SET balance -= " << args[2] << " WHERE id = " << args[0] << "\n";
            std::cout << "[ProcedureEngine] UPDATE accounts SET balance += " << args[2] << " WHERE id = " << args[1] << "\n";
            std::cout << "[ProcedureEngine] COMMIT\n";
            std::cout << "[ProcedureEngine] Procedure " << name << " executed successfully\n";
            return true;
        }
        
        std::cout << "[ProcedureEngine] Executed procedure: " << name << "\n";
        return true;
    }
    
    bool drop_procedure(const std::string& name) {
        procedure_bodies_.erase(name);
        procedure_params_.erase(name);
        std::cout << "[ProcedureEngine] Dropped procedure: " << name << "\n";
        return true;
    }
    
    int get_procedure_count() const {
        return procedure_bodies_.size();
    }
    
    int get_parameter_count(const std::string& name) const {
        auto it = procedure_params_.find(name);
        if (it != procedure_params_.end()) {
            return it->second.size();
        }
        return 0;
    }
};

}

#endif
