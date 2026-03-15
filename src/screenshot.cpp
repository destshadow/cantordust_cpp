#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "screenshot.h"

bool Screenshot::save(const std::string& filepath,
                      const std::vector<uint8_t>& rgb,
                      int width, int height) {
    int result = stbi_write_png(
        filepath.c_str(),
        width, height,
        3,
        rgb.data(),
        width * 3
    );
    if (result)
        std::cout << "[Screenshot] Salvato: " << filepath << "\n";
    else
        std::cerr << "[Screenshot] Errore salvataggio: " << filepath << "\n";

    return result != 0;
}
