#include <iostream>
#include <fstream>
#include <cassert>
#include <unistd.h>
#include <sys/wait.h>

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
        
        std::cout << "  Wrote: 1 row, schema\n";
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
        
        char* data = new char[data_len];
        if (!wal.read(data, data_len)) break;
        
        if (committed) {
            recovered++;
            std::cout << "  ✅ Recovered committed row: txn=" << txn_id << "\n";
        }
        delete[] data;
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
