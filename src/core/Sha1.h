#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace mc {

// Lowercase hex SHA-1 of a file, or nullopt if it can't be read.
std::optional<std::string> sha1File(const std::filesystem::path& path);

} // namespace mc
