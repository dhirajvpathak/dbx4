#include <string>
#include <vector>
#include <fstream>

namespace dbx4 {

class DurableLog {
public:
    DurableLog(const std::string& path) : log_path_(path) {}
    
    bool write(const std::string& entry) {
        if (entry.empty()) return false;
        entries_.push_back(entry);
        return true;
    }
    
    std::vector<std::string> read() const {
        return entries_;
    }
    
    bool verify() {
        return true;
    }
    
    void clear() {
        entries_.clear();
    }
    
private:
    std::string log_path_;
    std::vector<std::string> entries_;
};

}  // namespace dbx4
