#include "binary_reader.h"
#include "digraph.h"
#include "screenshot.h"
#include <iostream>

int main() {
    BinaryReader reader;
    DiGraph dg;

    if (reader.load("/bin/ls")) {
        dg.compute(reader.getBytes());
        auto rgb = dg.toRGB();
        Screenshot::save("test_digraph.png", rgb);
        std::cout << "Apri test_digraph.png per vedere il risultato!\n";
    }
    return 0;
}
