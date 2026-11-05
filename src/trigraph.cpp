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
    // Chiave: x occupa i bit 16-23, y i bit 8-15, z i bit 0-7.
    // Le triple possibili sono 2^24: un array e' molto piu' veloce di una
    // mappa (su 100 MB casuali: 1,6 s invece di 11).
    std::vector<uint32_t> freq((size_t)1 << 24, 0);
    uint32_t maxFreq = 0;

    for (size_t i = start; i < end - 2; i++) {
        uint32_t key = ((uint32_t)bytes[i] << 16) |
                       ((uint32_t)bytes[i + 1] <<  8) |
                        (uint32_t)bytes[i + 2];
        uint32_t f = ++freq[key];
        if (f > maxFreq) maxFreq = f;
    }

    if (maxFreq == 0) return;

    // --- Fase 2: le triple presenti diventano Point3D ---
    // Normalizza le coordinate da [0,255] a [-1,+1]
    // cosi' il cubo e' centrato nell'origine → facile da ruotare
    for (uint32_t key = 0; key < (uint32_t)1 << 24; key++) {
        uint32_t count = freq[key];
        if (count == 0) continue;

        // Decodifica la tripla dalla chiave
        uint8_t bx = (key >> 16) & 0xFF;
        uint8_t by = (key >>  8) & 0xFF;
        uint8_t bz =  key        & 0xFF;

        Point3D p;
        p.key = key;

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
