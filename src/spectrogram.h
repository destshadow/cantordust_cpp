#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

// Spettrogramma: quanto e' forte ogni periodo, tratto per tratto.
//   X = posizione nella finestra (fino a 256 tratti)
//   Y = periodo in byte, dall'alto 2 fino a meta' della FFT, in scala log
//   luce = forza di quel periodo rispetto alla media del tratto (nero sotto
//          4 volte, pieno a 100 volte)
// Una riga orizzontale luminosa = un periodo che dura (record, righe di
// un'immagine, campioni audio); dove la riga cambia o si spegne cambia il
// tipo di dati. Su dati casuali o compressi resta tutto scuro.
//
// Per ogni tratto si analizzano `fftSize` byte presi al suo centro: su una
// finestra grande e' un campione, cosi' 100 MB costano quanto 1 MB.
// I byte meno la loro media, finestra di Hann (riduce le code dei picchi),
// FFT, potenza |X_k|^2. Il periodo p corrisponde alla frequenza k = N/p.
//
// Un periodo p accende anche p/2, p/3... (le armoniche): la riga piu'
// luminosa non e' sempre il periodo vero. baseRow() somma le armoniche per
// indicare quello di base.
class Spectrogram {
public:
    static constexpr int    SIZE       = 256;    // colonne e righe al massimo
    static constexpr double MIN_PERIOD = 2.0;
    static constexpr double FULL       = 2.0;    // log10: 100 volte la media = luce piena
    static constexpr float  DARK       = -9.0f;  // tratto costante: nessun periodo
    // Sotto 4 volte la media resta nero: sui dati casuali il rumore di una
    // fascia di frequenze arriva li' (si prende il massimo della fascia)
    static constexpr double NOISE      = 0.6;

    // fftSize viene ridotta alla potenza di 2 che sta nella finestra;
    // removeMean toglie a ogni riga la sua media su tutti i tratti, cosi'
    // risaltano i tratti diversi dagli altri
    void compute(const std::vector<uint8_t>& bytes, int fftSize = 1024, bool removeMean = false);
    void clear();

    std::vector<uint8_t> toRGB() const;          // SIZE x SIZE, nero fuori

    int    columns()   const { return m_cols; }
    int    fftSize()   const { return m_n; }
    double maxPeriod() const { return m_maxPeriod; }

    // Riga ↔ periodo (scala logaritmica); rowOf limita a [0, SIZE-1]
    double periodAt(int row) const;
    int    rowOf(double period) const;

    // Il tratto della colonna c: [columnStart(c), columnStart(c+1)) e i
    // byte analizzati, da segmentStart(c) per fftSize()
    size_t columnStart(int c) const;
    size_t segmentStart(int c) const;
    int    columnOf(size_t pos) const;

    // log10(potenza / media del tratto); DARK se il tratto non ha periodi
    float level(int col, int row) const;
    int   baseRow(int col) const;                // -1 se nessun periodo spicca

    // Il periodo intero vicino a `approx` (entro il 6%, almeno +-1) per cui
    // piu' byte di [from, from+len) sono uguali a quello `periodo` dopo.
    // Le righe sono in scala log e un periodo letto da una riga e' 23,9 o
    // 24,4: questo dice 24. Restituisce almeno 1.
    static int exactPeriod(const std::vector<uint8_t>& bytes, size_t from, size_t len,
                           double approx);

private:
    int    m_cols = 0, m_n = 0;
    size_t m_len  = 0;
    double m_maxPeriod = 0;
    std::vector<float>  m_level;                 // [riga * SIZE + colonna]
    std::vector<size_t> m_seg;
};
