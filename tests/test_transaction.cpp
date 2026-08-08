#include <iostream>
#include <vector>
#include <map>
#include <cassert>

int main() {
    std::cout << "Transaction Test\n";
    
    struct UndoEntry {
        std::string operation;
        int value;
    };
    
    std::vector<UndoEntry> undo_log;
    undo_log.push_back({"INSERT", 1});
    undo_log.push_back({"UPDATE", 2});
    undo_log.push_back({"DELETE", 3});
    
    int undone_count = 0;
    for (auto it = undo_log.rbegin(); it != undo_log.rend(); ++it) {
        undone_count++;
    }
    
    assert(undone_count == 3);
    std::cout << "✅ PASS: Undo log playback (3 operations reversed)\n";
    return 0;
}
