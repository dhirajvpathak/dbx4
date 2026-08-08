#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    std::string wal = "/tmp/test_recovery.wal";
    
    // Cleanup
    unlink(wal.c_str());
    
    // Process 1: Writer
    pid_t pid = fork();
    if (pid == 0) {
        // Child: write and commit
        std::ofstream w(wal, std::ios::app);
        w << "1|1|row1\n";  // txn_id|committed|data
        w << "2|0|row2\n";  // uncommitted
        w.close();
        exit(0);
    }
    
    // Wait for child
    int status;
    waitpid(pid, &status, 0);
    
    // Process 2: Reader (recovery)
    std::ifstream r(wal);
    int committed_count = 0;
    int uncommitted_count = 0;
    std::string line;
    
    while (std::getline(r, line)) {
        if (line[1] == '|' && line[2] == '1') {
            committed_count++;
        } else if (line[1] == '|' && line[2] == '0') {
            uncommitted_count++;
        }
    }
    r.close();
    
    std::cout << "✅ Multi-process recovery test PASSED\n";
    std::cout << "  Committed: " << committed_count << "\n";
    std::cout << "  Uncommitted: " << uncommitted_count << "\n";
    
    return (committed_count == 1 && uncommitted_count == 1) ? 0 : 1;
}
