#pragma once
#include "ngram_model.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Classifier: a quale tipo di contenuto somiglia un tratto di byte (codice
// x86, ARM, testo, compresso, uno script...). E' una somiglianza statistica
// sui 4-grammi, non un riconoscimento: dice "somiglia a", mai "e'".
//
// Il campione "compresso" e' quasi uniforme: da' a ogni 4-gramma la stessa
// probabilita', piccola ma mai minima. Cosi' vincerebbe su qualunque tratto
// con 4-grammi che gli altri campioni non hanno visto (cifre in UTF-16, dati
// di un formato raro). Per questo le classi `noise` vincono solo se i byte
// sembrano davvero casuali (NOISE_MIN_BITS).
//
// Il funzionamento e' quello del CantorDust di Battelle (ClassifierModel.java):
// ogni etichetta si decide su CONTEXT byte centrati sul punto, perche' su
// 12 byte (9 quattro-grammi) loro misurano il 77% di risposte giuste e su 64
// il 91%. Qui si classifica solo dove serve (un punto per pixel), non tutto
// il file a blocchi.
//
// I campioni stanno in templates/: <nome>.template sono quelli originali di
// Battelle (tools/get_templates.sh), <nome>.bin quelli fatti su questa
// macchina (tools/make_templates.sh). Se ci sono entrambi si usano entrambi.
// Senza nessun campione il Classifier non c'e' e il resto funziona uguale.
struct ContentClass {
    const char* file;        // nome dei file in templates/
    const char* label;       // per l'Inspector
    uint8_t r, g, b;
    bool noise = false;      // dati quasi casuali: compresso e PNG (non "immagine":
                             // e' un bitmap, entropia media 2,8 bit su 64 byte)
};

class ClassifierModel {
public:
    static constexpr size_t CONTEXT    = 64;          // byte per decidere
    static constexpr size_t MAX_SAMPLE = 4u << 20;    // byte letti per classe
    static constexpr size_t CHUNK      = 64u << 10;   // a pezzi da 64 KB
    // Sotto questa entropia (bit, su 64 byte; massimo 6) un tratto non e'
    // casuale e le classi `noise` non possono vincere. Misurato: 64 byte
    // casuali non scendono sotto 5,4; testo UTF-16 sta intorno a 3.
    static constexpr double NOISE_MIN_BITS = 5.0;

    static int count();
    static const ContentClass& info(int cls);         // cls fuori: "?" grigio

    // Legge i campioni e allena il modello: circa un secondo, va chiamata
    // FUORI dal thread dell'interfaccia. Dopo, il modello si legge soltanto
    // e isReady() diventa vero. Restituisce quante classi hanno un campione.
    int loadTemplates(const std::string& dir);

    // Per i test: un campione per classe, nell'ordine di info()
    void train(const std::vector<std::vector<uint8_t>>& samples);

    bool isReady() const { return m_ready.load(std::memory_order_acquire); }
    bool hasClass(int cls) const { return isReady() && m_model.hasData(cls); }

    // A cosa somiglia il contenuto intorno a data[at]; -1 se non pronto o
    // se ci sono meno di 4 byte
    int classAt(const uint8_t* data, size_t size, size_t at) const;

private:
    NGramModel        m_model;
    std::atomic<bool> m_ready{false};
};
