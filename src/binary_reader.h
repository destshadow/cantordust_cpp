#pragma once
#include <string>
#include <vector>
#include <cstdint>

class BinaryReader {
public:
    bool load(const std::string& filepath);

    const std::vector<uint8_t>& getBytes() const { return m_bytes; }
    size_t getSize() const { return m_bytes.size(); }
    const std::string& getFilepath() const { return m_filepath; }
    bool isLoaded() const { return m_loaded; }

private:
    std::vector<uint8_t> m_bytes;
    std::string m_filepath;
    bool m_loaded = false;
};
