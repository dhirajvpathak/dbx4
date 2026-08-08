#include <string>
#include <map>
#include <memory>

namespace dbx4 {

struct MatchResult {
    float confidence = -1.0f;
    std::string match_id;
};

class AgentIntegration {
public:
    MatchResult find_best_match(const std::string&) {
        MatchResult best;
        best.confidence = -1.0f;
        return best;
    }
};

}
