#include "binary_reader.h"
#include <fstream>
#include <iostream>

bool BinaryReader::load(const std::string& filepath) {
    m_filepath = filepath;
    m_loaded   = false;
    m_bytes.clear();

    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[BinaryReader] Errore: impossibile aprire " << filepath << "\n";
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    m_bytes.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(m_bytes.data()), size)) {
        std::cerr << "[BinaryReader] Errore: lettura fallita per " << filepath << "\n";
        return false;
    }

    m_loaded = true;
    std::cout << "[BinaryReader] Caricato: " << filepath
              << " (" << size << " bytes)\n";
    return true;
}
