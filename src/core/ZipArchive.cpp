#include "core/ZipArchive.h"

#include "core/StringUtil.h"

#include <miniz/miniz.h>

#include <cstdio>
#include <stdexcept>

namespace mc {

struct ZipArchive::Impl {
    mz_zip_archive zip {};
    FILE* file = nullptr;

    int locate(std::string_view name) const
    {
        const std::string terminated(name);
        return mz_zip_reader_locate_file(const_cast<mz_zip_archive*>(&zip), terminated.c_str(), nullptr, MZ_ZIP_FLAG_CASE_SENSITIVE);
    }
};

ZipArchive::ZipArchive(const std::filesystem::path& archivePath)
    : path(archivePath)
    , impl(std::make_unique<Impl>())
{
    // _wfopen: miniz's own file API takes narrow paths, which break on non-ASCII user folders.
    if (_wfopen_s(&impl->file, archivePath.c_str(), L"rb") != 0 || !impl->file)
        throw std::runtime_error("Can't open " + toUtf8(archivePath.wstring()));

    _fseeki64(impl->file, 0, SEEK_END);
    const int64_t size = _ftelli64(impl->file);
    _fseeki64(impl->file, 0, SEEK_SET);

    if (!mz_zip_reader_init_cfile(&impl->zip, impl->file, static_cast<mz_uint64>(size), 0)) {
        const std::string error = mz_zip_get_error_string(mz_zip_get_last_error(&impl->zip));
        std::fclose(impl->file);
        impl->file = nullptr;
        throw std::runtime_error("Not a valid zip archive: " + toUtf8(archivePath.wstring()) + " (" + error + ")");
    }
}

ZipArchive::~ZipArchive()
{
    mz_zip_reader_end(&impl->zip);
    if (impl->file)
        std::fclose(impl->file);
}

bool ZipArchive::contains(std::string_view name) const
{
    std::lock_guard lock(mutex);
    return impl->locate(name) >= 0;
}

std::optional<std::vector<uint8_t>> ZipArchive::read(std::string_view name) const
{
    std::lock_guard lock(mutex);

    const int index = impl->locate(name);
    if (index < 0)
        return std::nullopt;

    mz_zip_archive_file_stat stat;
    if (!mz_zip_reader_file_stat(&impl->zip, static_cast<mz_uint>(index), &stat) || stat.m_is_directory)
        return std::nullopt;

    std::vector<uint8_t> data(static_cast<size_t>(stat.m_uncomp_size));
    if (!mz_zip_reader_extract_to_mem(&impl->zip, static_cast<mz_uint>(index), data.data(), data.size(), 0))
        return std::nullopt;
    return data;
}

std::vector<std::string> ZipArchive::list(std::string_view prefix) const
{
    std::lock_guard lock(mutex);

    std::vector<std::string> names;
    const mz_uint count = mz_zip_reader_get_num_files(&impl->zip);
    for (mz_uint i = 0; i < count; ++i) {
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&impl->zip, i, &stat) || stat.m_is_directory)
            continue;
        const std::string_view name(stat.m_filename);
        if (name.starts_with(prefix))
            names.emplace_back(name);
    }
    return names;
}

} // namespace mc
