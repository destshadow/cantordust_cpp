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
    // Calcola il canvas 256x256 interpretando i byte come pixel
    // width: larghezza in pixel del canvas (default 256)
    void compute(const std::vector<uint8_t>& bytes,
                 BppMode mode  = BppMode::BPP_8,
                 int     width = 256);
    void clear();

    // Buffer RGB 256x256 pronto per rendering
    std::vector<uint8_t> toRGB() const { return m_rgb; }

    BppMode getMode()  const { return m_mode; }
    int     getWidth() const { return m_width; }

    // Nome della modalita' per la UI
    static const char* modeName(BppMode m);

private:
    // Decodifica un singolo pixel per ogni modalita'
    void decodePixel8 (const std::vector<uint8_t>& b,
                       size_t idx, uint8_t& r, uint8_t& g, uint8_t& bu);
    void decodePixel16(const std::vector<uint8_t>& b,
                       size_t idx, uint8_t& r, uint8_t& g, uint8_t& bu);
    void decodePixel24(const std::vector<uint8_t>& b,
                       size_t idx, uint8_t& r, uint8_t& g, uint8_t& bu);
    void decodePixel32(const std::vector<uint8_t>& b,
                       size_t idx, uint8_t& r, uint8_t& g, uint8_t& bu);
    void decodePixel64(const std::vector<uint8_t>& b,
                       size_t idx, uint8_t& r, uint8_t& g, uint8_t& bu);

    std::vector<uint8_t> m_rgb;
    BppMode              m_mode  = BppMode::BPP_8;
    int                  m_width = 256;
};
