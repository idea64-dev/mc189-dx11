#pragma once

// Original: net/minecraft/client/resources/Locale.java, I18n.java, Language.java

#include "core/JavaString.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace mc {

class IResourceManager;

class Locale {
public:
    void loadLocaleDataFiles(const IResourceManager& resourceManager, const std::vector<std::string>& languageList);
    bool isUnicode() const { return unicode; }
    std::string formatMessage(const std::string& translateKey, std::span<const std::string> parameters) const;
    const std::unordered_map<std::string, std::string>& getProperties() const { return properties; }

    // Locale's rewrite of "%d", "%.2f", "%1$d"... to "%s" / "%1$s" (pattern %(\d+\$)?[\d\.]*[df]).
    static std::string rewriteFormatSpecifiers(std::string_view value);

private:
    void checkUnicode();
    void loadLocaleData(std::string_view contents);
    const std::string& translateKeyPrivate(const std::string& translateKey) const;

    std::unordered_map<std::string, std::string> properties;
    bool unicode = false;
};

class I18n {
public:
    static void setLocale(const Locale* locale) { i18nLocale = locale; }

    template <class... Args>
    static std::string format(const std::string& translateKey, const Args&... parameters)
    {
        const std::vector<std::string> converted { java::toString(parameters)... };
        return i18nLocale ? i18nLocale->formatMessage(translateKey, converted) : translateKey;
    }

private:
    friend class StatCollector;
    static inline const Locale* i18nLocale = nullptr;
};

// Java: net/minecraft/util/StatCollector over StringTranslate. LanguageManager hands StringTranslate the current
// locale's table on every reload, so on the client it reads the same map as I18n, but returns the raw value.
class StatCollector {
public:
    static std::string translateToLocal(const std::string& key);
    static bool canTranslate(const std::string& key);
};

class Language {
public:
    Language(std::string languageCode, std::string region, std::string name, bool bidirectional)
        : languageCode(std::move(languageCode))
        , region(std::move(region))
        , name(std::move(name))
        , bidirectional(bidirectional)
    {
    }

    const std::string& getLanguageCode() const { return languageCode; }
    bool isBidirectional() const { return bidirectional; }
    std::string toString() const { return name + " (" + region + ")"; }

    bool operator==(const Language& other) const { return languageCode == other.languageCode; }
    bool operator<(const Language& other) const { return languageCode < other.languageCode; }

private:
    std::string languageCode;
    std::string region;
    std::string name;
    bool bidirectional;
};

} // namespace mc
