#pragma once
#include "view_mode.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

// Panoramica: la finestra divisa in tratti uguali, e per ogni tratto una
// manciata di indicatori. Sono descrizioni, non giudizi: dicono com'e' fatto
// un tratto, non se e' sospetto.
class Overview {
public:
    static constexpr int COLS = 624;        // tratti al massimo: uno per pixel

    struct Cell {
        float entropy = 0;    // 0..1 (8 bit = 1)
        float jump    = 0;    // quanto cambia l'entropia dal tratto prima, 0..1
        float text    = 0;    // quota di byte stampabili
        float zeros   = 0;    // quota di 0x00
        float ff      = 0;    // quota di 0xFF
        float period  = 0;    // forza della ripetizione piu' netta, 0..1
        uint8_t periodLen = 0;    // ogni quanti byte (1..MAX_PERIOD), 0 = nessuna
        float lowBits = 0;    // i bit bassi sembrano casuali: 0..1, vedi lowBitRandomness
    };

    static constexpr int    MAX_PERIOD     = 64;
    static constexpr size_t PERIOD_SAMPLE  = 4096;     // byte guardati per tratto
    static constexpr size_t LOWBIT_MIN     = 65536;    // byte per il test dei bit bassi

    void compute(const std::vector<uint8_t>& bytes);
    void clear() { m_cells.clear(); m_len = 0; }

    const std::vector<Cell>& cells() const { return m_cells; }
    // Il tratto della finestra coperto dalla cella c: [start, end)
    size_t cellStart(size_t c) const { return m_cells.empty() ? 0 : c * m_len / m_cells.size(); }
    size_t cellOf(size_t offset) const { return partOf(offset, m_len, m_cells.size()); }

    // Test del chi-quadro di Westfeld e Pfitzmann (2000) su un istogramma: se
    // il bit meno significativo e' casuale, i valori 2k e 2k+1 compaiono lo
    // stesso numero di volte. Restituisce la probabilita' che sia cosi'.
    // 0 se i byte sono pochi o se TUTTO il byte e' casuale (dati compressi o
    // cifrati: li' non dice nulla).
    // Attenzione: nelle foto e nell'audio i bit bassi sono casuali di loro
    // (rumore del sensore). E' un indizio solo dove non te lo aspetti: codice,
    // testo, tabelle.
    static float lowBitRandomness(const std::array<uint64_t, 256>& hist);

    // La ripetizione piu' netta nei byte [a, b): quanta parte dei byte e'
    // uguale a quello `len` posizioni dopo, oltre a quanto capita per caso
    static float periodicity(const uint8_t* a, const uint8_t* b, uint8_t& len);

private:
    std::vector<Cell> m_cells;
    size_t m_len = 0;
};
