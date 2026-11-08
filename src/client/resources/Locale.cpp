#include "client/resources/Locale.h"

#include "client/resources/IResourceManager.h"
#include "client/resources/IResourcePack.h"

#include <cctype>
#include <format>

namespace mc {

std::string Locale::rewriteFormatSpecifiers(std::string_view value)
{
    std::string out;
    const auto isDigit = [](char c) { return c >= '0' && c <= '9'; };

    for (size_t i = 0; i < value.size();) {
        if (value[i] == '%') {
            // Optional "(\d+\$)", then "[\d.]*", then "[df]".
            size_t j = i + 1;
            std::string_view group;
            size_t k = j;
            while (k < value.size() && isDigit(value[k]))
                ++k;
            if (k > j && k < value.size() && value[k] == '$') {
                group = value.substr(j, k + 1 - j);
                j = k + 1;
            }
            size_t m = j;
            while (m < value.size() && (isDigit(value[m]) || value[m] == '.'))
                ++m;
            if (m < value.size() && (value[m] == 'd' || value[m] == 'f')) {
                out += '%';
                out += group;
                out += 's';
                i = m + 1;
                continue;
            }
            if (!group.empty()) {
                // Without the group: "[\d.]*" must then reach a d/f straight after the '%'.
                size_t n = i + 1;
                while (n < value.size() && (isDigit(value[n]) || value[n] == '.'))
                    ++n;
                if (n < value.size() && (value[n] == 'd' || value[n] == 'f')) {
                    out += "%s";
                    i = n + 1;
                    continue;
                }
            }
        }
        out += value[i];
        ++i;
    }
    return out;
}

void Locale::loadLocaleDataFiles(const IResourceManager& resourceManager, const std::vector<std::string>& languageList)
{
    properties.clear();
    for (const std::string& language : languageList) {
        const std::string path = std::format("lang/{}.lang", language);
        for (const std::string& domain : resourceManager.getResourceDomains()) {
            try {
                for (const auto& resource : resourceManager.getAllResources(ResourceLocation(domain, path))) {
                    const auto& data = resource->getInputStream();
                    loadLocaleData(std::string_view(reinterpret_cast<const char*>(data.data()), data.size()));
                }
            } catch (const ResourceNotFoundException&) {
                // Java: IOException ignored
            }
        }
    }
    checkUnicode();
}

void Locale::checkUnicode()
{
    unicode = false;
    size_t wide = 0;
    size_t total = 0;
    for (const auto& [key, value] : properties) {
        // Count UTF-16 units >= 256 the way String.charAt would see them.
        const std::wstring utf16 = [&] {
            std::wstring w;
            for (size_t i = 0; i < value.size();) {
                const auto lead = static_cast<unsigned char>(value[i]);
                uint32_t cp = lead;
                size_t len = 1;
                if ((lead >> 5) == 0x6) { cp = lead & 0x1F; len = 2; }
                else if ((lead >> 4) == 0xE) { cp = lead & 0x0F; len = 3; }
                else if ((lead >> 3) == 0x1E) { cp = lead & 0x07; len = 4; }
                for (size_t k = 1; k < len && i + k < value.size(); ++k)
                    cp = (cp << 6) | (static_cast<unsigned char>(value[i + k]) & 0x3F);
                i += len;
                if (cp >= 0x10000) {
                    w += static_cast<wchar_t>(0xD800 + ((cp - 0x10000) >> 10));
                    w += static_cast<wchar_t>(0xDC00 + ((cp - 0x10000) & 0x3FF));
                } else {
                    w += static_cast<wchar_t>(cp);
                }
            }
            return w;
        }();
        total += utf16.size();
        for (const wchar_t unit : utf16) {
            if (unit >= 256)
                ++wide;
        }
    }
    const float f = static_cast<float>(wide) / static_cast<float>(total);
    unicode = static_cast<double>(f) > 0.1;
}

void Locale::loadLocaleData(std::string_view contents)
{
    // IOUtils.readLines: split on \n, \r\n or \r.
    size_t start = 0;
    while (start <= contents.size()) {
        size_t end = contents.find_first_of("\r\n", start);
        const std::string_view line = contents.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start);

        if (!line.empty() && line[0] != '#') {
            const size_t eq = line.find('=');
            if (eq != std::string_view::npos) {
                std::string key(line.substr(0, eq));
                properties[std::move(key)] = rewriteFormatSpecifiers(line.substr(eq + 1));
            }
        }

        if (end == std::string_view::npos)
            break;
        start = end + 1;
        if (contents[end] == '\r' && start < contents.size() && contents[start] == '\n')
            ++start;
    }
}

const std::string& Locale::translateKeyPrivate(const std::string& translateKey) const
{
    const auto it = properties.find(translateKey);
    return it == properties.end() ? translateKey : it->second;
}

std::string Locale::formatMessage(const std::string& translateKey, std::span<const std::string> parameters) const
{
    const std::string& s = translateKeyPrivate(translateKey);
    if (auto formatted = java::format(s, parameters))
        return std::move(*formatted);
    return "Format error: " + s;
}

std::string StatCollector::translateToLocal(const std::string& key)
{
    if (I18n::i18nLocale == nullptr)
        return key;
    const auto& properties = I18n::i18nLocale->getProperties();
    const auto it = properties.find(key);
    return it == properties.end() ? key : it->second;
}

std::string StatCollector::translateToLocalFormatted(const std::string& key, std::span<const std::string> parameters)
{
    const std::string s = translateToLocal(key);
    const auto formatted = java::format(s, parameters);
    return formatted ? *formatted : "Format error: " + s;
}

bool StatCollector::canTranslate(const std::string& key)
{
    return I18n::i18nLocale != nullptr && I18n::i18nLocale->getProperties().contains(key);
}

} // namespace mc
