#include <iostream>
#include <cassert>

void test_suite_1() {
    std::cout << "✅ TEST-SUITE-1\n";
    assert(true);
}

int main() {
    test_suite_1();
    return 0;
}
