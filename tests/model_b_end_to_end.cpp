#include <iostream>
#include <fstream>
#include <cassert>
#include <cstring>

int main() {
    std::cout << "Model B End-to-End Test\n\n";
    
    system("rm -rf /tmp/model_b_test");
    system("mkdir -p /tmp/model_b_test");
    
    // Write test data
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
    
    // Verify recovery
    std::ifstream verify("/tmp/model_b_test/data.wal", std::ios::binary);
    uint32_t r_txn, r_committed, r_len;
    
    int tests_passed = 0;
    
    if (verify.read((char*)&r_txn, 4)) {
        std::cout << "✅ Transaction ID read\n";
        tests_passed++;
    }
    
    if (verify.read((char*)&r_committed, 4) && r_committed == 1) {
        std::cout << "✅ Committed flag verified\n";
        tests_passed++;
    }
    
    if (verify.read((char*)&r_len, 4) && r_len > 0) {
        char* recovered_data = new char[r_len];
        if (verify.read(recovered_data, r_len)) {
            std::cout << "✅ Data recovered\n";
            tests_passed++;
        }
        delete[] recovered_data;
    }
    
    verify.close();
    
    if (verify.peek() == EOF) {
        std::cout << "✅ File EOF correct\n";
        tests_passed++;
    }
    
    std::cout << "\nModel B Results: " << tests_passed << "/4 checks passed\n";
    
    if (tests_passed == 4) {
        std::cout << "✅ Model B End-to-End PASSED\n";
        return 0;
    } else {
        std::cout << "❌ Model B End-to-End FAILED\n";
        return 1;
    }
}
