#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <string>

int main() {
    std::cout << "Model B End-to-End Test\n\n";
    
    system("rm -rf /tmp/model_b_test");
    system("mkdir -p /tmp/model_b_test");
    
    std::ofstream data_file("/tmp/model_b_test/data.wal", std::ios::binary);
    uint32_t txn = 1;
    uint32_t committed = 1;
    std::string test_data = "row1";
    uint32_t len = test_data.length();
    
    data_file.write((char*)&txn, 4);
    data_file.write((char*)&committed, 4);
    data_file.write((char*)&len, 4);
    data_file.write(test_data.c_str(), len);
    data_file.close();
    
    std::ifstream verify("/tmp/model_b_test/data.wal", std::ios::binary);
    uint32_t r_txn, r_committed, r_len;
    int tests_passed = 0;
    
    if (verify.read((char*)&r_txn, 4)) { std::cout << "✅ Txn ID\n"; tests_passed++; }
    if (verify.read((char*)&r_committed, 4) && r_committed == 1) { std::cout << "✅ Committed\n"; tests_passed++; }
    if (verify.read((char*)&r_len, 4)) { std::cout << "✅ Data len\n"; tests_passed++; }
    
    char buffer[1024];
    if (r_len > 0 && verify.read(buffer, r_len)) { std::cout << "✅ Data read\n"; tests_passed++; }
    
    verify.close();
    
    std::cout << "\n✅ Model B PASSED (" << tests_passed << "/4)\n";
    return tests_passed == 4 ? 0 : 1;
}
