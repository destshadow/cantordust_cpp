#include "binary_reader.h"
#include "digraph.h"
#include <iostream>

int main() {
    BinaryReader reader;
    DiGraph dg;

    if (reader.load("/bin/ls")) {
        dg.compute(reader.getBytes());
        std::cout << "DiGraph calcolato!\n";
        std::cout << "Valore massimo: " << dg.getMax() << "\n";
        std::cout << "Buffer RGB size: " << dg.toRGB().size() << "\n";
    }
    return 0;
}
