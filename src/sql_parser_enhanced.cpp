#include <string>
#include <algorithm>
#include <cctype>

namespace dbx4 {

class EnhancedSQLParser {
public:
    static std::string trim(const std::string& s) {
        auto start = s.begin();
        while (start != s.end() && std::isspace(*start)) {
            ++start;
        }
        
        auto end = s.end();
        do {
            --end;
        } while (std::distance(start, end) > 0 && std::isspace(*end));
        
        return std::string(start, end + 1);
    }
    
    static std::string to_upper(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }
};

}  // namespace dbx4
