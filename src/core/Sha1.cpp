#include "core/Sha1.h"

#include <Windows.h>
#include <bcrypt.h>

#include <array>
#include <format>
#include <fstream>
#include <vector>

namespace mc {

std::optional<std::string> sha1File(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return std::nullopt;

    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA1_ALGORITHM, nullptr, 0)))
        return std::nullopt;

    std::optional<std::string> result;
    BCRYPT_HASH_HANDLE hash = nullptr;
    if (BCRYPT_SUCCESS(BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0))) {
        std::vector<char> buffer(1 << 16);
        bool ok = true;
        while (ok && stream) {
            stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            const auto count = static_cast<ULONG>(stream.gcount());
            if (count > 0)
                ok = BCRYPT_SUCCESS(BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer.data()), count, 0));
        }

        std::array<UCHAR, 20> digest {};
        if (ok && BCRYPT_SUCCESS(BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0))) {
            std::string hex;
            for (const UCHAR byte : digest)
                hex += std::format("{:02x}", byte);
            result = std::move(hex);
        }
        BCryptDestroyHash(hash);
    }
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return result;
}

} // namespace mc
