#include "file_navigator.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

void FileNavigator::setData(const std::vector<uint8_t>& bytes,
                             size_t windowSize) {
    m_totalSize  = bytes.size();
    m_windowSize = std::min(windowSize, m_totalSize);
    m_step       = m_windowSize / 16; // step = 1/16 della finestra
    m_start      = 0;
    m_end        = m_windowSize;
}

void FileNavigator::moveForward() {
    if (m_start + m_step + m_windowSize > m_totalSize) {
        // Vai alla fine senza sforare
        m_end   = m_totalSize;
        m_start = (m_totalSize > m_windowSize)
                  ? m_totalSize - m_windowSize : 0;
    } else {
        m_start += m_step;
        m_end    = m_start + m_windowSize;
    }
}

void FileNavigator::moveBackward() {
    if (m_start < m_step) {
        m_start = 0;
    } else {
        m_start -= m_step;
    }
    m_end = m_start + m_windowSize;
}

// Salta a posizione normalizzata [0.0, 1.0]
// Usato dalla scrollbar: l'utente clicca → calcola la posizione
void FileNavigator::jumpTo(float normalized) {
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    size_t maxStart = (m_totalSize > m_windowSize)
                      ? m_totalSize - m_windowSize : 0;
    m_start = static_cast<size_t>(normalized * maxStart);
    m_end   = m_start + m_windowSize;
    if (m_end > m_totalSize) m_end = m_totalSize;
}

float FileNavigator::getNormalized() const {
    if (m_totalSize <= m_windowSize) return 0.0f;
    size_t maxStart = m_totalSize - m_windowSize;
    return static_cast<float>(m_start) / static_cast<float>(maxStart);
}

// Formatta la posizione come stringa esadecimale
// Es: "0x001A2B3C — 0x002A3B4C"
std::string FileNavigator::getPositionString() const {
    std::ostringstream oss;
    oss << "0x" << std::uppercase << std::hex
        << std::setfill('0') << std::setw(8) << m_start
        << " - 0x"
        << std::setfill('0') << std::setw(8) << m_end;
    return oss.str();
}

// Ritorna una copia dei byte nella finestra corrente
// La copia e' necessaria perche' gli algoritmi lavorano
// su std::vector<uint8_t> indipendente
std::vector<uint8_t> FileNavigator::getWindow(
    const std::vector<uint8_t>& bytes) const {

    if (m_start >= bytes.size()) return {};
    size_t end = std::min(m_end, bytes.size());
    return std::vector<uint8_t>(bytes.begin() + m_start,
                                bytes.begin() + end);
}
