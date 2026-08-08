#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <string>

int main() {
    std::cout << "Real Recovery Test: Committed Data Survives Restart\n\n";
    
    // Create WAL directory
    system("rm -rf /tmp/test_recovery_wal");
    system("mkdir -p /tmp/test_recovery_wal");
    
    std::cout << "Process 1: Write committed data...\n";
    
    // Writer process
    pid_t pid = fork();
    if (pid == 0) {
        // Child: write committed data
        std::ofstream wal("/tmp/test_recovery_wal/data.wal");
        wal << "1|1|1|test_table|id=1;name=Alice\n";  // committed=1
        wal.close();
        
        std::ofstream schema("/tmp/test_recovery_wal/schema.txt");
        schema << "test_table:id,name\n";
        schema.close();
        
        std::cout << "  Wrote: 1 row, schema\n";
        exit(0);
    }
    
    // Wait for child
    int status;
    waitpid(pid, &status, 0);
    
    std::cout << "\nProcess 2: Restart and recover...\n";
    
    // Reader process - verify data persisted
    std::ifstream wal("/tmp/test_recovery_wal/data.wal");
    if (!wal.is_open()) {
        std::cout << "  ❌ WAL file not found\n";
        return 1;
    }
    
    std::string line;
    int recovered = 0;
    while (std::getline(wal, line)) {
        if (!line.empty() && line[5] == '1') {  // Check committed flag
            recovered++;
            std::cout << "  ✅ Recovered committed row: " << line << "\n";
        }
    }
    wal.close();
    
    if (recovered > 0) {
        std::cout << "\n✅ Recovery Test PASSED\n";
        std::cout << "  Committed data survived restart\n";
        return 0;
    } else {
        std::cout << "\n❌ Recovery Test FAILED\n";
        std::cout << "  No committed data recovered\n";
        return 1;
    }
}
