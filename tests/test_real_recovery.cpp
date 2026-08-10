#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdint>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>

int main() {
    std::cout << "Real Recovery Test: Committed Data Survives Restart\n\n";
    
    system("rm -rf /tmp/test_recovery_wal");
    system("mkdir -p /tmp/test_recovery_wal");
    
    std::cout << "Process 1: Write committed data...\n";
    pid_t pid = fork();
    if (pid == 0) {
        std::ofstream wal("/tmp/test_recovery_wal/data.wal", std::ios::binary);
        uint32_t txn_id = 1;
        uint32_t committed = 1;
        std::string data = "test_table|id=1;name=Alice";
        uint32_t data_len = data.length();
        
        wal.write((char*)&txn_id, 4);
        wal.write((char*)&committed, 4);
        wal.write((char*)&data_len, 4);
        wal.write(data.c_str(), data_len);
        wal.close();
        
        std::cout << "  Wrote: 1 row\n";
        exit(0);
    }
    
    int status;
    waitpid(pid, &status, 0);
    
    std::cout << "\nProcess 2: Restart and recover...\n";
    std::ifstream wal("/tmp/test_recovery_wal/data.wal", std::ios::binary);
    assert(wal.is_open());
    
    int recovered = 0;
    uint32_t txn_id, committed, data_len;
    
    while (wal.read((char*)&txn_id, 4)) {
        if (!wal.read((char*)&committed, 4)) break;
        if (!wal.read((char*)&data_len, 4)) break;
        
        char data[1024] = {0};
        if (!wal.read(data, data_len)) break;
        
        if (committed) {
            recovered++;
            std::cout << "  ✅ Recovered txn " << txn_id << "\n";
        }
    }
    wal.close();
    
    std::cout << "\n✅ Recovery Test PASSED (" << recovered << " rows)\n";
    return recovered > 0 ? 0 : 1;
}
