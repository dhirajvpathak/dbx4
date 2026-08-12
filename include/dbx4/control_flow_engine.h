#ifndef DBX4_CONTROL_FLOW_ENGINE_H
#define DBX4_CONTROL_FLOW_ENGINE_H

#include <string>
#include <vector>
#include <iostream>

namespace dbx4 {

class ControlFlowEngine {
public:
    ControlFlowEngine() {}
    
    // IF/ELSE conditional execution
    bool execute_if_else(bool condition, const std::string& true_branch, const std::string& false_branch) {
        if (condition) {
            std::cout << "[ControlFlow] Executing IF branch: " << true_branch << "\n";
            return true;
        } else {
            std::cout << "[ControlFlow] Executing ELSE branch: " << false_branch << "\n";
            return true;
        }
    }
    
    // WHILE loop
    int execute_while(int max_iterations, const std::string& body) {
        int iterations = 0;
        while (iterations < max_iterations) {
            std::cout << "[ControlFlow] WHILE iteration " << (iterations + 1) << ": " << body << "\n";
            iterations++;
        }
        std::cout << "[ControlFlow] WHILE loop completed " << iterations << " iterations\n";
        return iterations;
    }
    
    // FOR loop
    int execute_for(int start, int end, const std::string& body) {
        int iterations = 0;
        for (int i = start; i <= end; ++i) {
            std::cout << "[ControlFlow] FOR iteration " << i << ": " << body << "\n";
            iterations++;
        }
        std::cout << "[ControlFlow] FOR loop completed " << iterations << " iterations\n";
        return iterations;
    }
    
    // LOOP (infinite until EXIT)
    int execute_loop_with_exit(int max_iterations, const std::string& body) {
        int iterations = 0;
        while (iterations < max_iterations) {
            std::cout << "[ControlFlow] LOOP iteration " << (iterations + 1) << ": " << body << "\n";
            iterations++;
            if (iterations >= max_iterations) {
                std::cout << "[ControlFlow] EXIT loop\n";
                break;
            }
        }
        return iterations;
    }
    
    // CASE/WHEN statement
    std::string execute_case_when(int value, const std::vector<std::pair<int, std::string>>& cases, const std::string& default_case) {
        for (const auto& [case_val, result] : cases) {
            if (value == case_val) {
                std::cout << "[ControlFlow] CASE " << value << " matched: " << result << "\n";
                return result;
            }
        }
        std::cout << "[ControlFlow] CASE default: " << default_case << "\n";
        return default_case;
    }
};

}

#endif
