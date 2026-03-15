#pragma once
#include <vector>
#include <cstdint>
#include <string>

class FileNavigator {
public:
    // Imposta il file da navigare
    void setData(const std::vector<uint8_t>& bytes,
                 size_t windowSize = 65536); // default 64KB per finestra

    // Muovi la finestra avanti/indietro
    void moveForward();
    void moveBackward();

    // Salta a una posizione assoluta (0.0 = inizio, 1.0 = fine)
    void jumpTo(float normalized);

    // Getter
    size_t getStart()      const { return m_start; }
    size_t getEnd()        const { return m_end; }
    size_t getWindowSize() const { return m_windowSize; }
    size_t getTotalSize()  const { return m_totalSize; }

    // Posizione normalizzata [0.0, 1.0] per la scrollbar
    float  getNormalized() const;

    // Stringa per la UI: "0x1A2B — 0x3C4D"
    std::string getPositionString() const;

    // Slice dei bytes nella finestra corrente
    std::vector<uint8_t> getWindow(const std::vector<uint8_t>& bytes) const;

    bool isValid() const { return m_totalSize > 0; }

private:
    size_t m_start      = 0;
    size_t m_end        = 0;
    size_t m_windowSize = 65536;
    size_t m_totalSize  = 0;
    size_t m_step       = 4096; // quanto si sposta ad ogni click
};
