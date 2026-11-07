#include "client/renderer/texture/TextureUtil.h"

#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include <stb/stb_image.h>
#pragma warning(pop)

#include <limits>
#include <stdexcept>
#include <string>

namespace mc {

BufferedImage TextureUtil::readBufferedImage(std::span<const uint8_t> data)
{
    if (data.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        throw std::runtime_error("Image too large");

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* rgba = stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &width, &height, &channels, 4);
    if (!rgba)
        throw std::runtime_error(std::string("Couldn't decode image: ") + stbi_failure_reason());

    BufferedImage image;
    image.width = width;
    image.height = height;
    image.pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
    for (size_t i = 0; i < image.pixels.size(); ++i) {
        const stbi_uc* p = rgba + i * 4;
        image.pixels[i] = (static_cast<uint32_t>(p[3]) << 24) | (static_cast<uint32_t>(p[0]) << 16)
            | (static_cast<uint32_t>(p[1]) << 8) | static_cast<uint32_t>(p[2]);
    }
    stbi_image_free(rgba);
    return image;
}

} // namespace mc
