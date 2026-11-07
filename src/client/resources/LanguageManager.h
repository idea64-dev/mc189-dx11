#pragma once

// Original: net/minecraft/client/resources/LanguageManager.java (+ LanguageMetadataSection parsing)

#include "client/resources/IResourceManager.h"
#include "client/resources/Locale.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace mc {

class IResourcePack;

class LanguageManager : public IResourceManagerReloadListener {
public:
    explicit LanguageManager(std::string currentLanguage);

    void parseLanguageMetadata(const std::vector<std::shared_ptr<IResourcePack>>& resourcePacks);
    void onResourceManagerReload(const IResourceManager& resourceManager) override;

    bool isCurrentLocaleUnicode() const { return currentLocale.isUnicode(); }
    bool isCurrentLanguageBidirectional() const;
    void setCurrentLanguage(const Language& language) { currentLanguage = language.getLanguageCode(); }
    const Language* getCurrentLanguage() const;
    std::set<Language> getLanguages() const;

private:
    std::string currentLanguage;
    std::map<std::string, Language> languageMap;
    // Java: a static Locale shared by every LanguageManager; there is only ever one.
    static inline Locale currentLocale;
};

} // namespace mc
