#pragma once
#include <atomic>
#include <cstddef>

// Annullare un calcolo in corso. Chi lancia il calcolo passa una bandierina
// (Cancel::Scope), i cicli lunghi la guardano ogni tanto (Cancel::every) e,
// se e' alzata, escono lasciando il risultato a meta': chi ha chiesto di
// annullare lo butta via.
//
// La bandierina sta in una variabile del thread, cosi' le classi di calcolo
// non cambiano firma e, senza Scope (nei test, nel thread dell'interfaccia),
// non si annulla mai.
namespace Cancel {

inline thread_local const std::atomic<bool>* flag = nullptr;

// Quante volte questo thread ha guardato la bandierina: serve ai test, per
// verificare senza cronometro che un calcolo la guardi abbastanza spesso
inline thread_local size_t polls = 0;

inline bool requested() {
    if (!flag) return false;
    polls++;
    return flag->load(std::memory_order_relaxed);
}

// Per i cicli che avanzano di un passo alla volta: controlla ogni 64 KB.
// NON usarla se l'indice salta (i += n): i multipli di 64 KB verrebbero
// scavalcati quasi sempre e il ciclo non si fermerebbe. Li' serve Poll.
inline bool every(size_t i) { return (i & 0xFFFF) == 0 && requested(); }

// Per i cicli dove l'indice salta: controlla quando ha percorso 64 KB
// dall'ultimo controllo, qualunque sia il passo
class Poll {
public:
    bool at(size_t i) {
        if (i < m_next) return false;
        m_next = i + 0x10000;
        return requested();
    }
private:
    size_t m_next = 0;
};

struct Scope {
    explicit Scope(const std::atomic<bool>* f) { flag = f; }
    ~Scope() { flag = nullptr; }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
};

} // namespace Cancel
