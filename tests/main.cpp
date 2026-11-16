#include "tests.h"

int main() {
    testGolden();     // per primo: e' veloce, e `make golden` esce subito dopo
    testCore();
    testRaw();
    testAutocorr();
    testBytes();
    testStrings();
    testSignatures();
    testOverview();
    testMetricMap();
    testNavigation();
    testParser();
    test3D();
    std::cout << "All core regression tests passed\n";
}
