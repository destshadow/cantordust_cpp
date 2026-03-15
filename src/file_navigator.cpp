#include "file_navigator.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

void FileNavigator::setData(const std::vector<uint8_t>& bytes,
                             size_t windowSize) {
    m_totalSize = bytes.size();
    if (windowSize == 0 || windowSize >= m_totalSize)
        m_windowSize = m_totalSize;
    else
        m_windowSize = windowSize;
    m_step  = std::max((size_t)4096, m_windowSize / 16);
    m_start = 0;
    m_end   = m_windowSize;
}

// Reset: torna a mostrare il file intero
void FileNavigator::resetWindow(const std::vector<uint8_t>& bytes) {
    m_windowSize = m_totalSize;
    m_step       = std::max((size_t)4096, m_windowSize / 16);
    m_start      = 0;
    m_end        = m_windowSize;
}

void FileNavigator::moveForward() {
    // Se siamo a finestra intera → rimpicciolisci a 25%
    if (m_windowSize == m_totalSize) {
        m_windowSize = std::max((size_t)256, m_totalSize / 4);
        m_step       = std::max((size_t)4096, m_windowSize / 16);
        m_end        = m_start + m_windowSize;
        return;
    }
    if (m_start + m_step + m_windowSize > m_totalSize) {
        m_end   = m_totalSize;
        m_start = (m_totalSize > m_windowSize)
                  ? m_totalSize - m_windowSize : 0;
    } else {
        m_start += m_step;
        m_end    = m_start + m_windowSize;
    }
}

void FileNavigator::moveBackward() {
    if (m_start < m_step)
        m_start = 0;
    else
        m_start -= m_step;
    m_end = m_start + m_windowSize;
}

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

float FileNavigator::getWindowRatio() const {
    if (m_totalSize == 0) return 1.0f;
    return (float)m_windowSize / (float)m_totalSize;
}

std::string FileNavigator::getPositionString() const {
    std::ostringstream oss;
    oss << "0x" << std::uppercase << std::hex
        << std::setfill('0') << std::setw(8) << m_start
        << "-0x"
        << std::setfill('0') << std::setw(8) << m_end;
    if (m_windowSize == m_totalSize)
        oss << " (intero)";
    else {
        size_t pct = (m_windowSize * 100) / m_totalSize;
        oss << " (" << std::dec << pct << "%)";
    }
    return oss.str();
}

std::vector<uint8_t> FileNavigator::getWindow(
    const std::vector<uint8_t>& bytes) const {
    if (m_start >= bytes.size()) return {};
    size_t end = std::min(m_end, bytes.size());
    return std::vector<uint8_t>(bytes.begin() + m_start,
                                bytes.begin() + end);
}
