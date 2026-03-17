#pragma once
#include <vector>
#include <string>
#include <array>
#include <cstdint>
#include "ngram_model.h"
#include "exponential_notation.h"

// Classificatore NGram per sezioni di file binari
// Basato su ClassifierModel.java del codice originale CantorDust
//
// Funzionamento:
//   1. Carica template pre-calcolati per ogni classe
//   2. Per ogni blocco del file, costruisce un NGramModel
//   3. Confronta con tutti i template e sceglie la classe migliore
//
// Le 17 classi supportate (stesse del codice Java originale)
static constexpr int NUM_CLASSES = 17;
static const std::array<const char*, NUM_CLASSES> CLASS_NAMES = {{
    "arm4",         // ARM 32-bit little-endian
    "arm7",         // ARM 32-bit big-endian
    "ascii",        // Testo ASCII inglese
    "compressed",   // Dati compressi (zlib, lz4...)
    "java",         // Bytecode Java
    "mips",         // MIPS assembly
    "msil",         // .NET MSIL bytecode
    "ones",         // Sequenze di 0xFF
    "png",          // Dati PNG compressi
    "powerpc",      // PowerPC assembly
    "sparc_32",     // SPARC 32-bit
    "utf_16",       // Testo UTF-16
    "x64",          // x86-64 assembly
    "x86",          // x86 assembly
    "x86_padding",  // NOP padding x86
    "zeros",        // Sequenze di 0x00
    "embedded_image"// Immagini embedded
}};

// Colori per ogni classe — usati nell'overlay ColorClassifier
static const std::array<std::array<uint8_t,3>, NUM_CLASSES> CLASS_COLORS = {{
    {255, 100, 100},  // arm4        → rosso
    {255, 150,  50},  // arm7        → arancione
    {200, 255, 100},  // ascii       → verde chiaro
    {100, 100, 255},  // compressed  → blu
    {255, 255, 100},  // java        → giallo
    {100, 255, 200},  // mips        → turchese
    {200, 100, 255},  // msil        → viola
    { 50,  50,  50},  // ones        → grigio scuro
    {255, 200, 100},  // png         → oro
    {100, 200, 255},  // powerpc     → azzurro
    {255, 100, 200},  // sparc_32    → rosa
    {150, 255, 150},  // utf_16      → verde
    {255,  50,  50},  // x64         → rosso brillante
    {255, 150, 150},  // x86         → rosso chiaro
    { 80,  80,  80},  // x86_padding → grigio
    {200, 200, 200},  // zeros       → bianco
    {100, 255, 100},  // embedded    → verde brillante
}};

class ClassifierModel {
public:
    static constexpr int DEFAULT_GRAMS = 4;   // N per gli NGram
    static constexpr int BLOCK_SIZE    = 512;  // byte per blocco

    ClassifierModel();

    // Carica i template da file .bin nella cartella specificata
    // Ogni template e' un file binario pre-campionato dalla classe
    // Es: templates/x86.bin, templates/ascii.bin, ...
    bool loadTemplates(const std::string& templateDir,
                       int grams = DEFAULT_GRAMS);

    // Genera i template dai dati grezzi passati
    // Utile se non hai file template pre-calcolati
    void generateTemplates(
        const std::vector<std::vector<uint8_t>>& classSamples,
        int grams = DEFAULT_GRAMS);

    // Classifica un singolo blocco di byte
    // Ritorna l'indice della classe (0-16)
    int classify(const std::vector<uint8_t>& data,
                 int low, int high) const;

    // Classifica l'intero file dividendolo in blocchi
    void classifyFile(const std::vector<uint8_t>& data);

    // Classe del byte all'indice specificato
    int classAtIndex(size_t index) const;

    // Getters
    bool isReady()             const { return m_ready; }
    int  getBlockCount()       const { return (int)m_blockClassifications.size(); }
    const std::vector<int>&
         getClassifications()  const { return m_blockClassifications; }

    // Colore RGB per una classe
    static void getClassColor(int classIdx,
                               uint8_t& r, uint8_t& g, uint8_t& b);

private:
    std::vector<NGramModel> m_templates;
    std::vector<int>        m_blockClassifications;
    int                     m_grams = DEFAULT_GRAMS;
    bool                    m_ready = false;
};
