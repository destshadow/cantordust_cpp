#include "tests.h"

int main() {
    testCore();
    testRaw();
    testAutocorr();
    testBytes();
    testStrings();
    testMetricMap();
    testNavigation();
    testParser();
    test3D();
    std::cout << "All core regression tests passed\n";
}
