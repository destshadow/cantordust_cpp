#pragma once
#include <vector>
#include <cstdint>
#include <string>

class FileNavigator {
public:
    void setData(const std::vector<uint8_t>& bytes,
                 size_t windowSize = 0);  // 0 = file intero

    void selectRange(size_t start, size_t length);
    void moveForward();
    void moveBackward();
    void jumpTo(float normalized);

    // Reset: torna a mostrare il file intero
    void resetWindow(const std::vector<uint8_t>& bytes);

    size_t getStart()      const { return m_start; }
    size_t getEnd()        const { return m_end; }
    size_t getWindowSize() const { return m_windowSize; }
    size_t getTotalSize()  const { return m_totalSize; }

    float  getNormalized() const;
    float  getWindowRatio() const;  // windowSize/totalSize per scrollbar
    std::string getPositionString() const;

    std::vector<uint8_t> getWindow(
        const std::vector<uint8_t>& bytes) const;

    bool isValid() const { return m_totalSize > 0; }

private:
    size_t m_start      = 0;
    size_t m_end        = 0;
    size_t m_windowSize = 0;
    size_t m_totalSize  = 0;
    size_t m_step       = 4096;
};
