#include "client/resources/LanguageManager.h"

#include "client/resources/IResourcePack.h"
#include "core/Log.h"

#include <nlohmann/json.hpp>

namespace mc {

LanguageManager::LanguageManager(std::string language)
    : currentLanguage(std::move(language))
{
    I18n::setLocale(&currentLocale);
}

void LanguageManager::parseLanguageMetadata(const std::vector<std::shared_ptr<IResourcePack>>& resourcePacks)
{
    languageMap.clear();
    for (const auto& pack : resourcePacks) {
        try {
            const std::optional<nlohmann::json> section = pack->getPackMetadata("language");
            if (!section)
                continue;
            if (!section->is_object())
                throw std::runtime_error("language section is not an object");

            // LanguageMetadataSectionSerializer
            std::vector<Language> languages;
            for (const auto& [code, entry] : section->items()) {
                if (!entry.is_object())
                    throw std::runtime_error("Expected language to be a JsonObject");
                const std::string region = entry.at("region").get<std::string>();
                const std::string name = entry.at("name").get<std::string>();
                const bool bidirectional = entry.value("bidirectional", false);
                if (region.empty())
                    throw std::runtime_error("Invalid language->'" + code + "'->region: empty value");
                if (name.empty())
                    throw std::runtime_error("Invalid language->'" + code + "'->name: empty value");
                languages.emplace_back(code, region, name, bidirectional);
            }
            for (const Language& language : languages)
                languageMap.try_emplace(language.getLanguageCode(), language);
        } catch (const std::exception& e) {
            Log::warn("Unable to parse metadata section of resourcepack: {} ({})", pack->getPackName(), e.what());
        }
    }
}

void LanguageManager::onResourceManagerReload(const IResourceManager& resourceManager)
{
    std::vector<std::string> list { "en_US" };
    if (currentLanguage != "en_US")
        list.push_back(currentLanguage);
    currentLocale.loadLocaleDataFiles(resourceManager, list);
    // Java: StringTranslate.replaceWith(currentLocale.properties) for the integrated server (stage 9).
}

bool LanguageManager::isCurrentLanguageBidirectional() const
{
    const Language* language = getCurrentLanguage();
    return language && language->isBidirectional();
}

const Language* LanguageManager::getCurrentLanguage() const
{
    auto it = languageMap.find(currentLanguage);
    if (it == languageMap.end())
        it = languageMap.find("en_US");
    return it == languageMap.end() ? nullptr : &it->second;
}

std::set<Language> LanguageManager::getLanguages() const
{
    std::set<Language> languages;
    for (const auto& [code, language] : languageMap)
        languages.insert(language);
    return languages;
}

} // namespace mc
