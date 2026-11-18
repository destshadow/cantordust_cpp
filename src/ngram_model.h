#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

// Modelli a 4-grammi di piu' classi di contenuto (x86, testo, compresso...),
// tutti in una sola tabella. Serve al Classifier: un tratto di byte va alla
// classe che lo rende piu' probabile.
//
// Come nella versione riscritta del CantorDust di Battelle (NGramModel.java,
// agosto 2026), invece di una tabella con una voce per ogni 4-gramma visto:
//   - ogni 4-gramma va in uno di 2^BUCKET_BITS secchi (hash murmur3): la
//     memoria non dipende da quanto e' grande il campione. Due 4-grammi rari
//     nello stesso secchio si confondono, ma decidono quelli comuni;
//   - si tengono i logaritmi: la probabilita' di un tratto e' una somma, non
//     un prodotto che va a zero (qui prima serviva ExponentialNotation);
//   - un 4-gramma mai visto non e' impossibile: si mescola una distribuzione
//     uniforme con peso fisso LAMBDA. Con il peso fisso tutte le classi hanno
//     lo stesso minimo; con un conteggio aggiunto (Laplace) le classi con il
//     campione piu' piccolo (zeri, 0xFF) vincerebbero quasi sempre.
//
// In piu' rispetto a Battelle: la tabella e' ordinata per secchio, con tutte
// le classi una accanto all'altra, cosi' un 4-gramma costa una sola lettura
// di memoria per tutte le classi invece di una per classe.
class NGramModel {
public:
    static constexpr int    N           = 4;
    static constexpr int    BUCKET_BITS = 20;          // 4 MB per classe
    static constexpr size_t BUCKETS     = size_t(1) << BUCKET_BITS;
    static constexpr double LAMBDA      = 0.1;

    explicit NGramModel(int classes = 0);

    // Allenamento: si aggiungono i campioni (anche piu' pezzi per classe; un
    // 4-gramma non scavalca due pezzi), poi finish(). Dopo, solo letture.
    void add(int cls, const uint8_t* p, size_t n);
    // Un gramma con un peso: per i campioni che sono gia' un modello
    // (gramma → probabilita'), si passa probabilita' x numero di grammi
    void addGram(int cls, uint32_t gram, double weight);
    void finish();

    int  classes() const { return m_classes; }
    bool hasData(int cls) const;

    // La classe che spiega meglio [p, p+n); -1 se n < N o nessuna classe ha
    // dati. Se `score` non e' nullo riceve il punteggio di ogni classe.
    int best(const uint8_t* p, size_t n, std::vector<double>* score = nullptr) const;

private:
    int m_classes = 0;
    std::vector<float>    m_counts;      // solo durante l'allenamento
    std::vector<double>   m_total;       // 4-grammi visti per classe
    std::vector<float>    m_logP;        // [secchio * classi + classe]
};
