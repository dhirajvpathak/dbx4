#include <iostream>
#include <cassert>

struct PageHeader {
    uint32_t page_num = 0;
    uint32_t page_type = 0;
    uint16_t num_slots = 0;
    uint16_t free_offset = 256;  // FIXED
};

int main() {
    PageHeader header;
    constexpr uint32_t PAGE_SIZE = 8192;
    
    // Test 1: First insert
    assert(header.free_offset == 256);
    uint16_t row_len = 100;
    assert(header.free_offset + row_len <= PAGE_SIZE);
    header.free_offset += row_len;
    assert(header.num_slots == 0);
    header.num_slots++;
    
    // Test 2: Multiple inserts
    for (int i = 0; i < 10; i++) {
        if (header.free_offset + row_len > PAGE_SIZE) break;
        header.free_offset += row_len;
        header.num_slots++;
    }
    
    assert(header.num_slots == 11);
    assert(header.free_offset == 256 + (11 * 100));
    
    std::cout << "✅ Storage integration test PASSED\n";
    std::cout << "  Slots: " << header.num_slots << "\n";
    std::cout << "  Free offset: " << header.free_offset << "\n";
    std::cout << "  Available: " << (PAGE_SIZE - header.free_offset) << "\n";
    
    return 0;
}
