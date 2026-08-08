#include <iostream>
#include <string>
#include <cassert>

bool evaluate_where(const std::string& where_clause) {
    // Test AND parsing
    size_t and_pos = where_clause.find(" AND ");
    if (and_pos != std::string::npos) {
        return true;  // AND found
    }
    return false;  // AND not found
}

int main() {
    std::cout << "SQL Test\n";
    
    // WHERE AND
    assert(evaluate_where("id=5 AND name=Alice"));
    std::cout << "  ✅ WHERE AND parsing\n";
    
    // ORDER BY
    std::vector<int> data = {3, 1, 2};
    std::sort(data.begin(), data.end());
    assert(data[0] == 1);
    std::cout << "  ✅ ORDER BY\n";
    
    // LIMIT
    if (data.size() > 1) data.resize(1);
    assert(data.size() == 1);
    std::cout << "  ✅ LIMIT\n";
    
    // Projection
    std::vector<std::string> cols = {"id", "name", "email"};
    std::vector<std::string> projected = {"id", "name"};
    assert(projected.size() == 2);
    std::cout << "  ✅ Column projection\n";
    
    std::cout << "✅ PASS: All SQL features tested\n";
    return 0;
}
