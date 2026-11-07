#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mc {

// Read-only zip/jar access (client jar, zipped resource packs). Entry names are case-sensitive, like
// java.util.zip.ZipFile. Thread-safe.
class ZipArchive {
public:
    // Throws std::runtime_error if the file can't be opened or isn't a zip archive.
    explicit ZipArchive(const std::filesystem::path& path);
    ~ZipArchive();

    ZipArchive(const ZipArchive&) = delete;
    ZipArchive& operator=(const ZipArchive&) = delete;

    bool contains(std::string_view name) const;
    std::optional<std::vector<uint8_t>> read(std::string_view name) const;
    // Names of all file entries starting with `prefix`.
    std::vector<std::string> list(std::string_view prefix = {}) const;

    const std::filesystem::path& getPath() const { return path; }

private:
    struct Impl;
    std::filesystem::path path;
    std::unique_ptr<Impl> impl;
    mutable std::mutex mutex;
};

} // namespace mc
