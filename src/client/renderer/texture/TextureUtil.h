#pragma once

// Original: net/minecraft/client/renderer/texture/TextureUtil.java (image decoding so far; uploads come with
// the D3D11 texture code in ROADMAP stage 2.1).

#include "client/renderer/texture/BufferedImage.h"

#include <cstdint>
#include <span>

namespace mc {

class TextureUtil {
public:
    // Java: ImageIO.read. Throws std::runtime_error on undecodable data.
    // Known difference: ImageIO gamma-shifts greyscale PNGs through a linear grey colour space in getRGB();
    // vanilla 1.8.9 ships none (only RGB, RGBA and palette PNGs), resource packs might. Revisit in stage 3.
    static BufferedImage readBufferedImage(std::span<const uint8_t> data);
};

} // namespace mc
