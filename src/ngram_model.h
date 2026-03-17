#pragma once
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <string>
#include "vector_n.h"
#include "exponential_notation.h"

// Modello probabilistico NGram
// Conta le occorrenze di ogni sequenza di N byte
// e le converte in probabilita'
//
// Uso:
//   NGramModel model(data, 4);        // NGram di 4 byte
//   NGramModel tmpl(template_data, 4); // template pre-calcolato
//   auto score = model.evaluate(tmpl); // quanto data somiglia a tmpl?
class NGramModel {
public:
    // Costruisce il modello sull'intero buffer
    NGramModel(const std::vector<uint8_t>& data, int n);

    // Costruisce il modello su una finestra [startIndex, startIndex+length)
    NGramModel(const std::vector<uint8_t>& data,
               int startIndex, int length, int n);

    // Valuta quanto questo modello somiglia a templateModel
    // Ritorna un punteggio: piu' alto = piu' simile
    // Usa ExponentialNotation per evitare underflow
    ExponentialNotation evaluate(const NGramModel& templateModel) const;

    int  getN()           const { return m_n; }
    int  getModelEntries()const { return m_modelEntries; }
    size_t getModelSize() const { return m_model.size(); }

    std::string toString() const;

private:
    void generateModel(const std::vector<uint8_t>& data,
                       int startIndex, int length, int n);

    int m_n            = 0;
    int m_modelEntries = 0;

    // HashMap: sequenza di N byte → probabilita' (frequenza / totale)
    std::unordered_map<VectorN, double, VectorNHash, VectorNEqual> m_model;

    // Frequenza minima per NGram non trovati nel template
    // Evita moltiplicazione per 0 che azzererebbe tutto il punteggio
    static constexpr double MIN_FREQ_COEFF = 1.0;
    static constexpr long   MIN_FREQ_EXP   = -20;
};
