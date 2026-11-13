#ifndef LAKOT_LOCALIZATION_H
#define LAKOT_LOCALIZATION_H

#include <cstdint>
#include <initializer_list>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lakot
{

using TextArgs = std::initializer_list<std::pair<std::string_view, std::string>>;

struct LanguageInfo
{
    std::string code;
    std::string displayName;
};

class Localization
{
public:
    bool load(const std::string& pDataDir, std::string& pError);

    bool setLanguage(const std::string& pCode);
    const std::string& getLanguage() const;
    std::vector<LanguageInfo> getAvailableLanguages() const;

    std::string text(std::string_view pKey) const;
    std::string format(std::string_view pKey, TextArgs pArgs) const;
    std::string formatCount(std::string_view pKey, int64_t pCount, TextArgs pArgs = {}) const;
    std::string content(std::string_view pKey, std::string_view pEnglish) const;
    std::string formatDecimal(double pValue, int pDigits) const;

    void translateMarkup(std::string& pOut, const std::string& pIn) const;

    static std::string escapeMarkup(const std::string& pText);

private:
    using ArgList = std::vector<std::pair<std::string_view, std::string>>;

    struct Catalog
    {
        std::string displayName;
        std::string decimalSeparator{"."};
        std::unordered_map<std::string, std::string> texts;
        std::unordered_map<std::string, std::string> contents;
    };

    static constexpr const char* kBaseLanguage = "en";

    std::unordered_map<std::string, Catalog> mCatalogs;
    std::string mLanguage{kBaseLanguage};

    mutable std::mutex mReportedMutex;
    mutable std::unordered_set<std::string> mReported;

    const Catalog& getBase() const;
    const Catalog& getSelected() const;

    const std::string* findText(std::string_view pKey) const;
    static const std::string* findTextIn(const Catalog& pCatalog, std::string_view pKey);

    std::string expand(std::string_view pKey, const std::string& pTemplate, const ArgList& pArgs) const;
    void reportOnce(const std::string& pMessage) const;

    static bool loadLanguage(const std::string& pDirectory, Catalog& pCatalog, std::string& pError);
    static bool readFile(const std::string& pPath, std::unordered_map<std::string, std::string>& pTarget, std::string& pError);
};

}

#endif
