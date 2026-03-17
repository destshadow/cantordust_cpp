#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>

// Vettore di N byte — chiave per la HashMap dell'NGramModel
// Deve avere hash e uguaglianza ben definiti
class VectorN {
public:
    explicit VectorN(int n)
        : m_n(n), m_data(n, 0) {}

    VectorN(const uint8_t* data, int n)
        : m_n(n), m_data(data, data + n) {}

    uint8_t getAt(int idx)      const { return m_data[idx]; }
    void    setAt(int idx, uint8_t val) { m_data[idx] = val; }
    int     getN()              const { return m_n; }

    bool operator==(const VectorN& rhs) const {
        if (m_n != rhs.m_n) return false;
        return m_data == rhs.m_data;
    }

    // Hash: (hash << 5) ^ (hash >> 27) ^ byte
    // Stesso algoritmo del codice Java originale
    size_t hash() const {
        size_t h = (size_t)m_n;
        for (uint8_t b : m_data)
            h = (h << 5) ^ (h >> 27) ^ b;
        return h;
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << "< ";
        for (uint8_t b : m_data)
            oss << std::hex << std::setw(2)
                << std::setfill('0') << (int)b << " ";
        oss << ">";
        return oss.str();
    }

private:
    int                  m_n;
    std::vector<uint8_t> m_data;
};

// Functor hash per unordered_map
struct VectorNHash {
    size_t operator()(const VectorN& v) const { return v.hash(); }
};

struct VectorNEqual {
    bool operator()(const VectorN& a, const VectorN& b) const {
        return a == b;
    }
};
