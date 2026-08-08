#include <cstdint>
#include <vector>
#include <cstring>
#include <algorithm>

namespace dbx4 {

constexpr uint32_t PAGE_SIZE = 8192;
constexpr uint32_t PAGE_HEADER_SIZE = 256;
constexpr uint16_t MAX_SLOTS_PER_PAGE = 512;

struct SlotHeader {
    uint32_t offset;
    uint16_t length;
};

struct PageHeader {
    uint32_t page_num;
    uint32_t page_type;
    uint16_t num_slots;
    uint16_t free_offset;
    uint32_t checksum;
};

class StoragePage {
private:
    uint8_t data[PAGE_SIZE];
    PageHeader& header() { return *(PageHeader*)data; }
    
    SlotHeader* slot_headers() { 
        return (SlotHeader*)(data + sizeof(PageHeader));
    }
    
    uint8_t* payload_start() {
        return data + PAGE_HEADER_SIZE + (MAX_SLOTS_PER_PAGE * sizeof(SlotHeader));
    }
    
    uint32_t calculate_checksum(bool exclude_checksum_field) {
        uint32_t sum = 0;
        for (size_t i = 0; i < PAGE_SIZE; i++) {
            if (exclude_checksum_field && i >= offsetof(PageHeader, checksum) && 
                i < offsetof(PageHeader, checksum) + 4) {
                continue;
            }
            sum += data[i];
        }
        return sum;
    }
    
public:
    StoragePage(uint32_t page_num_val) {
        std::memset(data, 0, PAGE_SIZE);
        header().page_num = page_num_val;
        header().page_type = 1;
        header().num_slots = 0;
        header().free_offset = PAGE_HEADER_SIZE + (MAX_SLOTS_PER_PAGE * sizeof(SlotHeader));
        header().checksum = 0;
    }
    
    bool insert_row(const uint8_t* row_data, uint16_t row_len) {
        if (header().num_slots >= MAX_SLOTS_PER_PAGE) return false;
        if (header().free_offset + row_len > PAGE_SIZE) return false;
        
        // Record slot header
        SlotHeader* slot = &slot_headers()[header().num_slots];
        slot->offset = header().free_offset;
        slot->length = row_len;
        
        // Write row data
        std::memcpy(&data[header().free_offset], row_data, row_len);
        
        header().free_offset += row_len;
        header().num_slots++;
        
        return true;
    }
    
    std::vector<std::vector<uint8_t>> read_rows() {
        std::vector<std::vector<uint8_t>> rows;
        
        for (uint16_t i = 0; i < header().num_slots; i++) {
            SlotHeader* slot = &slot_headers()[i];
            std::vector<uint8_t> row(data + slot->offset, data + slot->offset + slot->length);
            rows.push_back(row);
        }
        
        return rows;
    }
    
    std::vector<uint8_t> serialize() {
        // Calculate checksum before serializing
        header().checksum = 0;
        header().checksum = calculate_checksum(true);
        
        // Return full page
        return std::vector<uint8_t>(data, data + PAGE_SIZE);
    }
    
    bool deserialize(const std::vector<uint8_t>& serialized) {
        if (serialized.size() != PAGE_SIZE) return false;
        
        std::memcpy(data, serialized.data(), PAGE_SIZE);
        
        // Verify checksum
        uint32_t stored_checksum = header().checksum;
        header().checksum = 0;
        uint32_t computed_checksum = calculate_checksum(true);
        
        return stored_checksum == computed_checksum;
    }
};

}
