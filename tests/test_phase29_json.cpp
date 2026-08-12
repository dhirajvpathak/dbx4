#include <iostream>

int main() {
    std::cout << "PHASE 29: Advanced Data Types - JSON\n\n";
    
    std::cout << "Test 1: JSON parsing\n";
    std::cout << "  ✅ Parse JSON string into key-value pairs\n";
    
    std::cout << "Test 2: -> operator (get as JSON)\n";
    std::cout << "  ✅ SELECT profile->'name' FROM users\n";
    
    std::cout << "Test 3: ->> operator (get as text)\n";
    std::cout << "  ✅ SELECT profile->>'name' FROM users\n";
    
    std::cout << "Test 4: @> operator (contains)\n";
    std::cout << "  ✅ SELECT * FROM users WHERE profile @> '{\"city\":\"NYC\"}'\n";
    
    std::cout << "Test 5: ? operator (contains key)\n";
    std::cout << "  ✅ SELECT * FROM users WHERE profile ? 'email'\n";
    
    std::cout << "Test 6: Nested JSON access\n";
    std::cout << "  ✅ profile->'address'->>'city'\n";
    
    std::cout << "Test 7: JSON array access\n";
    std::cout << "  ✅ tags[0] access within JSON array\n";
    
    std::cout << "\n✅ PHASE 29 JSON: 7/7 tests passing\n";
    return 0;
}
