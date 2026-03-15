#include "binary_reader.h"
#include <iostream>

int main() {
    BinaryReader reader;
    if (reader.load("/bin/ls")) {
        std::cout << "Bytes letti: " << reader.getSize() << "\n";
        std::cout << "Primi 4 bytes: ";
        for (int i = 0; i < 4; i++)
            std::cout << std::hex << (int)reader.getBytes()[i] << " ";
        std::cout << "\n";
    }
    return 0;
}
