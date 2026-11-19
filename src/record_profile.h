#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Profilo record: la finestra tagliata in record da `period` byte, a partire
// da `phase`, e per ogni posizione nel record cosa contiene su tutti i record.
//   X = posizione nel record, Y = valore del byte (0 in alto)
//   luce = quante volte quel valore sta in quella posizione (log, rispetto
//          alla colonna)
//   colore = tipo di campo della colonna (Field)
// Con il periodo giusto le colonne si separano: un campo fisso e' un punto
// solo, un contatore una diagonale di punti tutti uguali, il testo una
// fascia azzurra, i dati casuali una colonna piena.
// Senza conoscere il formato: dice "com'e' fatto il record", non cosa vuol
// dire.
class RecordProfile {
public:
    static constexpr int    SIZE        = 256;      // immagine SIZE x SIZE
    static constexpr int    MAX_PERIOD  = 4096;
    static constexpr size_t MAX_RECORDS = 65536;    // oltre si campiona

    enum class Field { NONE, FIXED, COUNTER, TEXT, FEW, VARIED, RANDOM };

    struct Column {
        Field   field    = Field::NONE;
        uint8_t top      = 0;       // il valore piu' frequente
        double  topShare = 0;       // la sua quota
        int     distinct = 0;       // valori diversi
        double  entropy  = 0;       // bit
        int     step     = 0;       // contatore: di quanto cresce (mod 256)
    };

    void compute(const std::vector<uint8_t>& bytes, int period, int phase);
    void clear();

    std::vector<uint8_t> toRGB() const;

    int    period()  const { return m_period; }
    int    phase()   const { return m_phase; }
    size_t records() const { return m_records; }   // record nella finestra
    size_t sampled() const { return m_sampled; }   // quanti ne sono stati contati
    bool   empty()   const { return m_records == 0; }

    // Posizione nel record della colonna di pixel x, e il contrario
    int byteAt(int x) const;
    int firstX(int byte) const;

    const Column& column(int byte) const;
    uint32_t count(int byte, uint8_t value) const;

    // Una lettera per byte: valore fisso in esadecimale, C contatore,
    // t testo, e pochi valori, . vario, ? casuale (max `maxBytes` byte)
    std::string pattern(int maxBytes) const;

    // Il tipo di una colonna dai conteggi dei valori e delle differenze fra
    // record consecutivi (256 + 256), su `samples` record (record_field.cpp)
    static Column classify(const uint32_t* values, const uint32_t* deltas, size_t samples);

    static const char* fieldName(Field f);
    static void fieldColor(Field f, uint8_t& r, uint8_t& g, uint8_t& b);

private:
    int    m_period = 0, m_phase = 0;
    size_t m_records = 0, m_sampled = 0;
    std::vector<uint32_t> m_counts;                 // [byte * 256 + valore]
    std::vector<uint32_t> m_deltas;                 // [byte * 256 + differenza]
    std::vector<Column>   m_columns;
};
