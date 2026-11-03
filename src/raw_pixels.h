#pragma once
#include <vector>
#include <cstdint>

// Modalita' di interpretazione dei byte come pixel
enum class BppMode {
    BPP_8  = 0,   // 1 byte  per pixel (grayscale/palette)
    BPP_16 = 1,   // 2 bytes per pixel (RGB565)
    BPP_24 = 2,   // 3 bytes per pixel (RGB standard)
    BPP_32 = 3,   // 4 bytes per pixel (RGBA)
    BPP_64 = 4,   // 8 bytes per pixel (come binvis Color64bpp)
};

class RawPixels {
public:
    static constexpr int MIN_WIDTH = 1;
    static constexpr int MAX_WIDTH = 2048;
    // Oltre questo numero di righe l'immagine viene campionata
    // (una riga ogni rowStep) per restare dentro una texture ragionevole
    static constexpr int MAX_ROWS  = 2048;

    // Interpreta bytes[start, end) come pixel, `width` pixel per riga,
    // saltando i primi `offset` byte (allineamento righe/canali).
    // L'altezza dipende da quanti byte ci sono.
    void compute(const std::vector<uint8_t>& bytes,
                 size_t start, size_t end,
                 BppMode mode   = BppMode::BPP_8,
                 int     width  = 256,
                 size_t  offset = 0);
    // Comodita': tutto il buffer
    void compute(const std::vector<uint8_t>& bytes,
                 BppMode mode = BppMode::BPP_8,
                 int width = 256, size_t offset = 0) {
        compute(bytes, 0, bytes.size(), mode, width, offset);
    }
    void clear();

    // Buffer RGB width x height
    const std::vector<uint8_t>& getRGB() const { return m_rgb; }
    std::vector<uint8_t> toRGB() const { return m_rgb; }

    BppMode getMode()    const { return m_mode; }
    int     getWidth()   const { return m_width; }
    int     getHeight()  const { return m_height; }
    size_t  getRowStep() const { return m_rowStep; }

    // Offset (relativo all'inizio della finestra) del pixel (x, y),
    // oppure SIZE_MAX se fuori immagine
    size_t byteOffsetAt(int x, int y) const;

    static size_t bytesPerPixel(BppMode m);
    static const char* modeName(BppMode m);

private:
    static void decodePixel(BppMode mode, const uint8_t* p,
                            uint8_t& r, uint8_t& g, uint8_t& b);

    std::vector<uint8_t> m_rgb;
    BppMode              m_mode    = BppMode::BPP_8;
    int                  m_width   = 0;
    int                  m_height  = 0;
    size_t               m_rowStep = 1;
    size_t               m_offset  = 0;
};
