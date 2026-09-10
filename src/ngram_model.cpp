#include "ngram_model.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <stdexcept>

NGramModel::NGramModel(const std::vector<uint8_t>& data, int n) {
    generateModel(data, 0, (int)data.size(), n);
}

NGramModel::NGramModel(const std::vector<uint8_t>& data,
                       int startIndex, int length, int n) {
    generateModel(data, startIndex, length, n);
}

// --- generateModel ---
// Scorre il buffer con una finestra scorrevole di N byte
// Conta le occorrenze di ogni sequenza unica
// Poi divide per il totale per ottenere probabilita'
void NGramModel::generateModel(const std::vector<uint8_t>& data,
                                int startIndex, int length, int n) {
    if (n <= 0 || startIndex < 0 || length < 0 ||
        size_t(startIndex) > data.size() || size_t(length) > data.size() - size_t(startIndex))
        throw std::invalid_argument("Invalid NGram range or order");
    m_n = n;

    // Conta le occorrenze di ogni NGram
    // Usa una mappa temporanea di conteggi (interi)
    std::unordered_map<VectorN, double,
                       VectorNHash, VectorNEqual> counts;

    int end = startIndex + length;
    for (int i = startIndex; i < end; i++) {
        // Controlla che ci siano abbastanza byte per un NGram completo
        if (n > end - i) break;

        // Costruisce il VectorN per questa finestra
        VectorN v(n);
        for (int k = 0; k < n; k++)
            v.setAt(k, data[i + k]);

        // Incrementa il conteggio
        auto it = counts.find(v);
        if (it != counts.end())
            it->second += 1.0;
        else
            counts[v] = 1.0;
    }

    // Calcola il totale degli NGram
    // modelEntries = lunghezza - n + 1 (numero di finestre possibili)
    m_modelEntries = length - n + 1;
    if (m_modelEntries < 0) m_modelEntries = 0;

    // Converti conteggi in probabilita'
    // p(NGram) = conteggio / modelEntries
    m_model.clear();
    for (auto& [key, count] : counts)
        m_model[key] = count / (double)m_modelEntries;
}

// --- evaluate ---
// Confronta questo modello con un template pre-calcolato
// Algoritmo:
//   Per ogni NGram nel modello corrente:
//     k = numero di occorrenze (conteggio)
//     Se l'NGram esiste nel template:
//       p = probabilita' nel template
//       score *= p^k   (quante volte questa sequenza appare nel template)
//     Altrimenti:
//       score *= MINIMUM (penalita' per sequenze non nel template)
//   Punteggio finale: piu' alto = piu' simile al template
ExponentialNotation NGramModel::evaluate(
    const NGramModel& templateModel) const {

    // Fattore minimo per NGram non trovati nel template
    // Evita che un singolo NGram sconosciuto azzeri tutto
    static const ExponentialNotation MIN_FACTOR(
        MIN_FREQ_COEFF, MIN_FREQ_EXP);

    // Punteggio iniziale = 1 (neutro per la moltiplicazione)
    ExponentialNotation score(1.0);

    for (auto& [v, prob] : m_model) {
        // Ricostruisci il conteggio dall'entropia
        // k = round(prob * modelEntries)
        int k = (int)(prob * m_modelEntries + 0.5);
        if (k <= 0) continue;

        auto it = templateModel.m_model.find(v);
        if (it != templateModel.m_model.end()) {
            // NGram trovato nel template
            // Calcola p^k con la potenza veloce
            ExponentialNotation pClass(it->second);
            score = score.multiply(MathUtils::fastPow(pClass, k));
        } else {
            // NGram non trovato — applica penalita' minima
            // Dividiamo per modelEntries per scalare la penalita'
            ExponentialNotation penalty =
                ExponentialNotation(1.0)
                .divide(ExponentialNotation((double)m_modelEntries))
                .multiply(MIN_FACTOR);
            score = score.multiply(MathUtils::fastPow(penalty, k));
        }
    }

    return score;
}

std::string NGramModel::toString() const {
    std::ostringstream oss;
    oss << m_modelEntries << " entries:\n";
    for (auto& [key, prob] : m_model) {
        int count = (int)(prob * m_modelEntries);
        oss << "\t" << key.toString()
            << " : " << count
            << " (" << prob << ")\n";
    }
    return oss.str();
}
