#include "binary_reader.h"
#include <fstream>
#include <iostream>

bool BinaryReader::load(const std::string& filepath) {

    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[BinaryReader] Errore: impossibile aprire " << filepath << "\n";
        return false;
    }

    std::streamsize size = file.tellg();
    if (size < 0) return false;
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> bytes;
    try { bytes.resize(static_cast<size_t>(size)); }
    catch (const std::exception& e) {
        std::cerr << "[BinaryReader] " << e.what() << "\n";
        return false;
    }
    if (!file.read(reinterpret_cast<char*>(bytes.data()), size)) {
        std::cerr << "[BinaryReader] Errore: lettura fallita per " << filepath << "\n";
        return false;
    }

    m_bytes = std::move(bytes);
    m_filepath = filepath;
    m_loaded = true;
    std::cout << "[BinaryReader] Caricato: " << filepath
              << " (" << size << " bytes)\n";
    return true;
}
