#pragma once

// Decoded image in the layout java.awt.image.BufferedImage.getRGB() returns: one int per pixel, 0xAARRGGBB,
// not premultiplied, rows top to bottom.

#include <cstdint>
#include <vector>

namespace mc {

struct BufferedImage {
    int width = 0;
    int height = 0;
    std::vector<uint32_t> pixels;

    uint32_t getRGB(int x, int y) const { return pixels[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)]; }
    void setRGB(int x, int y, uint32_t argb) { pixels[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)] = argb; }
};

} // namespace mc
