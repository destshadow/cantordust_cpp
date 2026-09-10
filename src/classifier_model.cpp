#include "classifier_model.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

ClassifierModel::ClassifierModel() {}

// --- loadTemplates ---
// Cerca i file template nella cartella specificata
// Formato atteso: <dir>/<classe>.bin
// Es: templates/x86.bin
bool ClassifierModel::loadTemplates(const std::string& templateDir,
                                     int grams) {
    if (grams <= 0) throw std::invalid_argument("NGram order must be positive");
    m_grams = grams;
    m_templates.clear();
    m_blockClassifications.clear();
    m_ready = false;

    int loaded = 0;
    for (int i = 0; i < NUM_CLASSES; i++) {
        std::string path = templateDir + "/" +
                           CLASS_NAMES[i] + ".bin";

        // Legge il file template
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f.is_open()) {
            std::cerr << "[Classifier] Template non trovato: "
                      << path << "\n";
            // Modello vuoto: questa classe non partecipa al confronto
            std::vector<uint8_t> empty;
            m_templates.emplace_back(empty, grams);
            continue;
        }

        auto fileSize = f.tellg();
        if (fileSize < grams || fileSize > std::numeric_limits<int>::max()) {
            m_templates.emplace_back(std::vector<uint8_t>{}, grams);
            continue;
        }
        size_t size = size_t(fileSize);
        f.seekg(0, std::ios::beg);
        std::vector<uint8_t> data(size);
        if (!f.read(reinterpret_cast<char*>(data.data()), size)) {
            m_templates.emplace_back(std::vector<uint8_t>{}, grams);
            continue;
        }

        m_templates.emplace_back(data, grams);
        loaded++;
        std::cout << "[Classifier] Template caricato: "
                  << CLASS_NAMES[i]
                  << " (" << size << " bytes, "
                  << m_templates.back().getModelSize()
                  << " NGram unici)\n";
    }

    m_ready = (loaded > 0);
    std::cout << "[Classifier] " << loaded << "/"
              << NUM_CLASSES << " template caricati\n";
    return m_ready;
}

// --- generateTemplates ---
// Genera template dai campioni forniti direttamente
// Utile per test senza file template
void ClassifierModel::generateTemplates(
    const std::vector<std::vector<uint8_t>>& classSamples,
    int grams) {

    if (grams <= 0) throw std::invalid_argument("NGram order must be positive");
    m_grams = grams;
    m_templates.clear();
    m_blockClassifications.clear();

    if (classSamples.size() > NUM_CLASSES)
        throw std::invalid_argument("Too many classifier classes");
    for (const auto& sample : classSamples)
        m_templates.emplace_back(sample, grams);

    m_ready = std::any_of(m_templates.begin(), m_templates.end(),
        [](const NGramModel& model) { return model.getModelSize() > 0; });
}

// --- classify ---
// Classifica un blocco [low, high) del buffer data
// Costruisce un NGramModel dal blocco e lo confronta
// con tutti i template — ritorna l'indice del piu' simile
int ClassifierModel::classify(const std::vector<uint8_t>& data,
                               int low, int high) const {
    if (m_templates.empty()) return -1;

    if (low < 0 || high < low || size_t(high) > data.size()) return -1;
    int len = high - low;
    if (len < m_grams) return -1;

    // Costruisce il modello per questo blocco
    NGramModel blockModel(data, low, len, m_grams);

    // Confronta con tutti i template
    // Tiene traccia del punteggio massimo
    ExponentialNotation bestScore(0.0);
    int bestClass = -1;

    for (int i = 0; i < (int)m_templates.size(); i++) {
        if (m_templates[i].getModelSize() == 0) continue;
        ExponentialNotation score =
            blockModel.evaluate(m_templates[i]);

        if (bestClass < 0 || score.greaterThan(bestScore)) {
            bestScore = score;
            bestClass = i;
        }
    }

    return bestClass;
}

// --- classifyFile ---
// Divide il file in blocchi da BLOCK_SIZE byte
// e classifica ognuno
void ClassifierModel::classifyFile(
    const std::vector<uint8_t>& data) {

    m_blockClassifications.clear();
    if (!m_ready || data.empty()) return;

    if (data.size() > size_t(std::numeric_limits<int>::max()))
        throw std::length_error("Classifier window exceeds supported size");
    size_t numBlocks = data.size() / BLOCK_SIZE + (data.size() % BLOCK_SIZE != 0);
    m_blockClassifications.resize(numBlocks);

    std::cout << "[Classifier] Classificazione di "
              << numBlocks << " blocchi...\n";

    for (size_t i = 0; i < numBlocks; i++) {
        int low  = (int)(i * BLOCK_SIZE);
        int high = (int)std::min(data.size(), size_t(low) + BLOCK_SIZE);
        m_blockClassifications[i] = classify(data, low, high);
    }

    std::cout << "[Classifier] Classificazione completata\n";
}

// --- classAtIndex ---
// Ritorna la classe del byte all'indice specificato
// Mappa l'indice byte → blocco → classificazione
int ClassifierModel::classAtIndex(size_t index) const {
    if (m_blockClassifications.empty()) return -1;
    size_t blockIdx = index / BLOCK_SIZE;
    if (blockIdx >= m_blockClassifications.size())
        return -1;  // oltre la fine del file
    return m_blockClassifications[blockIdx];
}

// --- getClassColor ---
// Ritorna il colore RGB associato alla classe
void ClassifierModel::getClassColor(int classIdx,
                                     uint8_t& r,
                                     uint8_t& g,
                                     uint8_t& b) {
    if (classIdx < 0 || classIdx >= NUM_CLASSES) {
        r = g = b = 128;
        return;
    }
    r = CLASS_COLORS[classIdx][0];
    g = CLASS_COLORS[classIdx][1];
    b = CLASS_COLORS[classIdx][2];
}
