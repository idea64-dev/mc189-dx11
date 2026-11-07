#include "util/ScreenShotHelper.h"

#include "client/renderer/DxStateManager.h"
#include "client/renderer/texture/TextureUtil.h"
#include "client/shader/Framebuffer.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "render/gl/GL.h"

#pragma warning(push, 0)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include <stb/stb_image_write.h>
#pragma warning(pop)

#include <ctime>
#include <format>
#include <fstream>
#include <vector>

namespace mc {

std::string ScreenShotHelper::saveScreenshot(const std::filesystem::path& gameDirectory, int width, int height, Framebuffer& buffer)
{
    return saveScreenshot(gameDirectory, {}, width, height, buffer);
}

std::string ScreenShotHelper::saveScreenshot(const std::filesystem::path& gameDirectory, const std::string& screenshotName, int /*width*/, int /*height*/, Framebuffer& buffer)
{
    try {
        const std::filesystem::path directory = gameDirectory / "screenshots";
        std::filesystem::create_directories(directory);

        const int width = buffer.framebufferTextureWidth;
        const int height = buffer.framebufferTextureHeight;
        DxStateManager::bindTexture(buffer.framebufferTexture);
        std::vector<uint32_t> pixelValues = GL::glGetTexImage(GL::GL_TEXTURE_2D, 0);
        if (pixelValues.size() != static_cast<size_t>(width) * static_cast<size_t>(height))
            throw std::runtime_error("framebuffer texture size mismatch");
        TextureUtil::processPixelValues(pixelValues, width, height);

        // BufferedImage.TYPE_INT_RGB: the alpha channel is dropped.
        const int j = buffer.framebufferTextureHeight - buffer.framebufferHeight;
        std::vector<uint8_t> rgb(static_cast<size_t>(buffer.framebufferWidth) * static_cast<size_t>(buffer.framebufferHeight) * 3);
        for (int k = j; k < buffer.framebufferTextureHeight; ++k) {
            for (int l = 0; l < buffer.framebufferWidth; ++l) {
                const uint32_t pixel = pixelValues[static_cast<size_t>(k) * static_cast<size_t>(buffer.framebufferTextureWidth) + static_cast<size_t>(l)];
                uint8_t* out = &rgb[(static_cast<size_t>(k - j) * static_cast<size_t>(buffer.framebufferWidth) + static_cast<size_t>(l)) * 3];
                out[0] = static_cast<uint8_t>(pixel >> 16);
                out[1] = static_cast<uint8_t>(pixel >> 8);
                out[2] = static_cast<uint8_t>(pixel);
            }
        }

        const std::filesystem::path file = screenshotName.empty() ? getTimestampedPNGFileForDirectory(directory) : directory / toWide(screenshotName);
        std::vector<uint8_t> png;
        const auto append = [](void* context, void* data, int size) {
            auto* bytes = static_cast<std::vector<uint8_t>*>(context);
            bytes->insert(bytes->end(), static_cast<uint8_t*>(data), static_cast<uint8_t*>(data) + size);
        };
        if (!stbi_write_png_to_func(append, &png, buffer.framebufferWidth, buffer.framebufferHeight, 3, rgb.data(), buffer.framebufferWidth * 3))
            throw std::runtime_error("PNG encoding failed");
        std::ofstream stream(file, std::ios::binary);
        stream.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
        if (!stream)
            throw std::runtime_error("can't write " + toUtf8(file.wstring()));

        const std::string name = toUtf8(file.filename().wstring());
        Log::info("Saved screenshot as {}", name);
        return "Saved screenshot as " + name;
    } catch (const std::exception& e) {
        Log::warn("Couldn't save screenshot ({})", e.what());
        return std::string("Couldn't save screenshot: ") + e.what();
    }
}

std::filesystem::path ScreenShotHelper::getTimestampedPNGFileForDirectory(const std::filesystem::path& directory)
{
    const std::time_t now = std::time(nullptr);
    std::tm local {};
    localtime_s(&local, &now);
    const std::string stamp = std::format("{:04}-{:02}-{:02}_{:02}.{:02}.{:02}", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
        local.tm_hour, local.tm_min, local.tm_sec);

    for (int i = 1;; ++i) {
        const std::filesystem::path file = directory / (stamp + (i == 1 ? "" : "_" + std::to_string(i)) + ".png");
        std::error_code ec;
        if (!std::filesystem::exists(file, ec))
            return file;
    }
}

} // namespace mc
