#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// Legge un campione del Classifier (templates/<nome>.template o .bin).
//
// I .template di Battelle sono di tre tipi (visto aprendoli, 2026-11-18):
//   1. oggetto .NET (BinaryFormatter) con il campione nell'array "data" e,
//      dopo, il modello gia' calcolato. Il campione e' 0,2-0,6 MB; il resto
//      del file, fino a 19 MB, e' il modello serializzato: usare il file
//      intero come campione (come fa il loro Java) allena anche su quello;
//   2. stessa intestazione, ma niente campione: solo il modello, voci da
//      16 byte (04 00 00 00, i 4 byte del gramma, probabilita' double).
//      Sono i cinque a 64 bit aggiunti nel 2026 (arm64, riscv64...);
//   3. byte grezzi senza intestazione (embedded_image).
// I nostri .bin sono sempre byte grezzi.
namespace TemplateFile {

struct Content {
    std::vector<uint8_t> bytes;                       // campione da contare
    std::vector<std::pair<uint32_t, double>> grams;   // oppure gramma → probabilita'
};

// `name` e' il nome della classe (serve a trovare l'inizio delle voci nel tipo 2)
Content parse(const std::vector<uint8_t>& file, const std::string& name);

// Il gramma come lo conta NGramModel: il primo byte nei bit alti
inline uint32_t gramKey(const uint8_t* p) {
    return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
}

} // namespace TemplateFile
