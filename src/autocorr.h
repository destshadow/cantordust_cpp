#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>

// Autocorrelazione dei byte: per ogni distanza k misura quanto
// bytes[i] somiglia a bytes[i+k] (differenza media assoluta).
// In un'immagine raw le righe consecutive si somigliano, quindi la
// distanza pari a una riga (larghezza * byte per pixel) da' un "pozzo"
// netto: e' cosi' che si trova la larghezza giusta per i raw pixel.
class Autocorr {
public:
    static constexpr int MAX_LAG     = 6144;   // 2048 pixel a 24bpp
    static constexpr int MAX_SAMPLES = 16384;  // posizioni campionate
    static constexpr int MAX_WIDTH   = 2048;   // come RawPixels::MAX_WIDTH

    void compute(const std::vector<uint8_t>& bytes, size_t start, size_t end);
    void compute(const std::vector<uint8_t>& bytes) {
        compute(bytes, 0, bytes.size());
    }

    // Somiglianza normalizzata in [0,1] per ogni distanza (indice = k).
    // L'indice 0 non e' usato. Vuoto se non c'erano abbastanza byte.
    const std::vector<float>& similarity() const { return m_sim; }
    int maxLag() const { return (int)m_sim.size() - 1; }

    // Larghezza (in pixel da `bpp` byte) piu' promettente: la riga di
    // un'immagine, preferendo la fondamentale ai suoi multipli, oppure la
    // dimensione di un record. 0 se non trovata.
    int bestWidth(size_t bpp, int minWidth = 16) const;

private:
    int imageWidth(size_t bpp, int minWidth) const;
    int bestPeriod(size_t bpp) const;          // autocorr_period.cpp

    std::vector<float> m_mad;   // differenza media assoluta per distanza
    std::vector<float> m_sim;   // 1 = piu' simile, 0 = meno simile
};
