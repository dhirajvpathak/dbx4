#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cassert>

namespace dbx4 {

constexpr uint32_t PAGE_SIZE = 8192;
constexpr uint32_t HEADER_SIZE = 256;
constexpr uint32_t SLOT_AREA_SIZE = 2048;  // Max 256 slots * 8 bytes
constexpr uint32_t PAYLOAD_START = HEADER_SIZE + SLOT_AREA_SIZE;
constexpr uint32_t PAYLOAD_SIZE = PAGE_SIZE - PAYLOAD_START;

struct SlotHeader {
    uint32_t offset;  // Offset within payload area (not page offset)
    uint16_t length;
    uint16_t reserved;
};

struct PageHeader {
    uint32_t page_num;
    uint32_t page_type;
    uint16_t num_slots;
    uint16_t free_offset;  // Offset within PAYLOAD_SIZE
    uint32_t checksum;
};

class StoragePage {
private:
    uint8_t data[PAGE_SIZE];
    
    PageHeader* header() {
        return (PageHeader*)data;
    }
    
    SlotHeader* slots() {
        return (SlotHeader*)(data + HEADER_SIZE);
    }
    
    uint8_t* payload() {
        return data + PAYLOAD_START;
    }
    
    uint32_t compute_checksum(bool exclude_checksum_field = true) {
        uint32_t sum = 0;
        for (uint32_t i = 0; i < PAGE_SIZE; i++) {
            // Skip checksum field itself
            if (exclude_checksum_field && 
                i >= offsetof(PageHeader, checksum) && 
                i < offsetof(PageHeader, checksum) + 4) {
                continue;
            }
            sum = (sum << 1) ^ data[i];  // Simple XOR-based checksum
        }
        return sum;
    }
    
public:
    StoragePage(uint32_t page_num_val = 0) {
        std::memset(data, 0, PAGE_SIZE);
        header()->page_num = page_num_val;
        header()->page_type = 1;
        header()->num_slots = 0;
        header()->free_offset = 0;
        header()->checksum = 0;
    }
    
    // Insert row into PAYLOAD area, record slot in SLOT area
    bool insert_row(const uint8_t* row_data, uint16_t row_len) {
        // Check bounds
        if (header()->num_slots >= 256) return false;  // Max slots
        if (header()->free_offset + row_len > PAYLOAD_SIZE) return false;
        
        // Record slot
        SlotHeader& slot = slots()[header()->num_slots];
        slot.offset = header()->free_offset;
        slot.length = row_len;
        slot.reserved = 0;
        
        // Write payload
        std::memcpy(payload() + header()->free_offset, row_data, row_len);
        
        header()->free_offset += row_len;
        header()->num_slots++;
        
        return true;
    }
    
    // Read row by index
    bool read_row(uint16_t slot_idx, std::vector<uint8_t>& out_row) {
        if (slot_idx >= header()->num_slots) return false;
        
        SlotHeader& slot = slots()[slot_idx];
        out_row.assign(payload() + slot.offset, payload() + slot.offset + slot.length);
        
        return true;
    }
    
    // Serialize: compute checksum, return full page
    std::vector<uint8_t> serialize() {
        header()->checksum = 0;
        header()->checksum = compute_checksum(true);
        
        return std::vector<uint8_t>(data, data + PAGE_SIZE);
    }
    
    // Deserialize: verify checksum, restore state
    bool deserialize(const std::vector<uint8_t>& serialized) {
        if (serialized.size() != PAGE_SIZE) return false;
        
        std::memcpy(data, serialized.data(), PAGE_SIZE);
        
        // Verify checksum
        uint32_t stored_checksum = header()->checksum;
        header()->checksum = 0;
        uint32_t computed = compute_checksum(true);
        
        return stored_checksum == computed;
    }
    
    // Verify page integrity
    bool verify() {
        uint32_t expected = compute_checksum(true);
        return header()->checksum == expected;
    }
};

}
