#include "trigraph.h"
#include <algorithm>
#include <cmath>

void TriGraph::clear() {
    m_points.clear();
}

void TriGraph::compute(const std::vector<uint8_t>& bytes,
                       size_t start, size_t end) {
    clear();
    if (bytes.size() < 3) return;

    // Se end == 0 usiamo tutto il file
    if (end == 0 || end > bytes.size())
        end = bytes.size();

    // Serve almeno 3 byte per una tripla
    if (end - start < 3) return;

    // --- Fase 1: conta le frequenze delle triple ---
    // Chiave: codifica (x,y,z) in un uint32_t
    // x occupa i bit 16-23, y i bit 8-15, z i bit 0-7
    // Questo garantisce una chiave unica per ogni tripla possibile
    std::unordered_map<uint32_t, uint32_t> freq;
    freq.reserve(1024); // pre-alloca per performance

    uint32_t maxFreq = 0;

    for (size_t i = start; i < end - 2; i++) {
        uint8_t x = bytes[i];
        uint8_t y = bytes[i + 1];
        uint8_t z = bytes[i + 2];

        // Codifica la tripla in un uint32_t
        uint32_t key = ((uint32_t)x << 16) |
                       ((uint32_t)y <<  8) |
                        (uint32_t)z;

        uint32_t f = ++freq[key];
        if (f > maxFreq) maxFreq = f;
    }

    if (maxFreq == 0) return;

    // --- Fase 2: converti la mappa in Point3D ---
    // Normalizza le coordinate da [0,255] a [-1,+1]
    // cosi' il cubo e' centrato nell'origine → facile da ruotare
    m_points.reserve(freq.size());

    for (auto& [key, count] : freq) {
        // Decodifica la tripla dalla chiave
        uint8_t bx = (key >> 16) & 0xFF;
        uint8_t by = (key >>  8) & 0xFF;
        uint8_t bz =  key        & 0xFF;

        Point3D p;

        // Normalizza in [-1, +1]
        p.x = (bx / 127.5f) - 1.0f;
        p.y = (by / 127.5f) - 1.0f;
        p.z = (bz / 127.5f) - 1.0f;

        // Intensita' con scala logaritmica
        // Esalta i pattern anche con poche occorrenze
        p.intensity = std::log1p((float)count) /
                      std::log1p((float)maxFreq);

        // Colore basato sulla posizione nel cubo
        // Questo crea il gradiente viola/magenta/blu
        // visibile negli screenshot originali
        p.r = static_cast<uint8_t>(p.intensity * bx);
        p.g = static_cast<uint8_t>(p.intensity * 50);
        p.b = static_cast<uint8_t>(p.intensity * bz);

        m_points.push_back(p);
    }
}
