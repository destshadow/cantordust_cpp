#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <iostream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../libs/stb_image_write.h"

class Screenshot {
public:
    static bool save(const std::string& filepath,
                     const std::vector<uint8_t>& rgb,
                     int width = 256, int height = 256) {
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
};
