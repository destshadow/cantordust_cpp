#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <iostream>
#include "../libs/stb_image_write.h"

class Screenshot {
public:
    static bool save(const std::string& filepath,
                     const std::vector<uint8_t>& rgb,
                     int width = 256, int height = 256);
};
