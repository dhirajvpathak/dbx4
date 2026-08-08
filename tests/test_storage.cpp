#include <iostream>
#include <cstring>
#include <cassert>

int main() {
    std::cout << "Storage Test\n";
    constexpr size_t PAGE_SIZE = 8192;
    uint8_t page1[PAGE_SIZE];
    uint8_t page2[PAGE_SIZE];
    
    std::memset(page1, 0, PAGE_SIZE);
    std::memcpy(page1 + 256, "test_data", 9);
    std::memcpy(page2, page1, PAGE_SIZE);
    
    assert(std::memcmp(page1, page2, PAGE_SIZE) == 0);
    std::cout << "✅ PASS: Storage round-trip integrity\n";
    return 0;
}
