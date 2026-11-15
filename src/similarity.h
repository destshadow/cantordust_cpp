#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

// Somiglianza: il file confrontato con se stesso (il "dotplot" classico).
// Il pixel (x, y) e' acceso se il tratto x e il tratto y della finestra hanno
// sequenze di byte in comune. Si legge cosi':
//
//   diagonale parallela a quella principale   lo stesso contenuto ripetuto
//                                             piu' avanti (copie, record)
//   quadrato pieno                            zona omogenea: stesso tipo di
//                                             dati, in qualunque ordine
//   riga o colonna vuota                      un tratto che non somiglia a
//                                             nient'altro
//
// Algoritmo di Church e Helfman, "Dotplot: A Program for Exploring
// Self-Similarity in Millions of Lines of Text and Code" (1993):
//   - gettoni: sequenze di GRAM byte; due posizioni combaciano se hanno lo
//     stesso gettone
//   - peso: una coincidenza vale 1/frequenza del gettone (le sequenze comuni
//     dicono poco)
//   - compressione: N posizioni in 256 celle, cella(i) = i * 256 / N
//   - approssimazione: un gettone si confronta al massimo con le MAX_FREQ
//     occorrenze successive (l'articolo scarta del tutto quelli piu' comuni)
// In piu', per i file grandi, si tengono solo i gettoni il cui hash cade in
// una classe su `step`: la scelta dipende dal contenuto, quindi una sequenza
// ripetuta viene tenuta in tutte le sue copie.
class Similarity {
public:
    static constexpr int    SIZE       = 256;
    static constexpr size_t GRAM       = 4;
    static constexpr size_t MAX_FREQ   = 32;
    static constexpr size_t MAX_TOKENS = 2000000;

    void compute(const std::vector<uint8_t>& bytes);
    void clear();
    std::vector<uint8_t> toRGB() const;

    // Peso delle coincidenze tra il tratto x e il tratto y (fuori diagonale)
    float at(int x, int y) const { return m_cells[(size_t)y * SIZE + x]; }

private:
    std::vector<float> m_cells = std::vector<float>((size_t)SIZE * SIZE, 0.0f);
};
