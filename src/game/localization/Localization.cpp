#include "Localization.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <SDL3/SDL.h>
#include <rapidjson/document.h>

using namespace lakot;

namespace
{
    constexpr const char* kMetaName = "meta.language_name";
    constexpr const char* kMetaDecimal = "meta.decimal_separator";
    constexpr const char* kWordJoiner = "\xE2\x81\xA0";

    bool isKeyCharacter(char pChar)
    {
        return (pChar >= 'a' && pChar <= 'z') || (pChar >= '0' && pChar <= '9') || pChar == '_' || pChar == '.';
    }
}

bool Localization::load(const std::string& pDataDir, std::string& pError)
{
    mCatalogs.clear();
    mReported.clear();
    mLanguage = kBaseLanguage;

    std::filesystem::path tRoot = std::filesystem::path(pDataDir) / "locale";
    std::error_code tCode;

    if (!std::filesystem::is_directory(tRoot / kBaseLanguage, tCode))
    {
        pError = "missing locale directory " + (tRoot / kBaseLanguage).string();
        return false;
    }

    for (const auto& tEntry : std::filesystem::directory_iterator(tRoot, tCode))
    {
        if (!tEntry.is_directory())
        {
            continue;
        }

        std::string tLanguage = tEntry.path().filename().string();
        Catalog tCatalog;
        std::string tError;

        if (!loadLanguage(tEntry.path().string(), tCatalog, tError))
        {
            if (tLanguage == kBaseLanguage)
            {
                pError = tError;
                return false;
            }

            SDL_Log("Localization: language %s skipped: %s", tLanguage.c_str(), tError.c_str());
            continue;
        }

        if (tCatalog.displayName.empty())
        {
            tCatalog.displayName = tLanguage;
        }

        mCatalogs[tLanguage] = std::move(tCatalog);
    }

    SDL_Log("Localization: %zu language(s), en has %zu keys", mCatalogs.size(), getBase().texts.size());
    return true;
}

bool Localization::loadLanguage(const std::string& pDirectory, Catalog& pCatalog, std::string& pError)
{
    std::filesystem::path tDirectory(pDirectory);

    for (const char* tFile : { "ui.json", "server.json" })
    {
        std::filesystem::path tPath = tDirectory / tFile;

        if (std::filesystem::exists(tPath) && !readFile(tPath.string(), pCatalog.texts, pError))
        {
            return false;
        }
    }

    std::filesystem::path tContent = tDirectory / "content.json";

    if (std::filesystem::exists(tContent) && !readFile(tContent.string(), pCatalog.contents, pError))
    {
        return false;
    }

    auto tName = pCatalog.texts.find(kMetaName);
    if (tName != pCatalog.texts.end())
    {
        pCatalog.displayName = tName->second;
        pCatalog.texts.erase(tName);
    }

    auto tDecimal = pCatalog.texts.find(kMetaDecimal);
    if (tDecimal != pCatalog.texts.end())
    {
        pCatalog.decimalSeparator = tDecimal->second;
        pCatalog.texts.erase(tDecimal);
    }

    return true;
}

bool Localization::readFile(const std::string& pPath, std::unordered_map<std::string, std::string>& pTarget, std::string& pError)
{
    std::ifstream tStream(pPath, std::ios::binary);

    if (!tStream)
    {
        pError = "cannot open " + pPath;
        return false;
    }

    std::stringstream tBuffer;
    tBuffer << tStream.rdbuf();

    rapidjson::Document tDocument;
    tDocument.Parse(tBuffer.str().c_str());

    if (tDocument.HasParseError() || !tDocument.IsObject())
    {
        pError = "invalid JSON in " + pPath;
        return false;
    }

    for (auto tMember = tDocument.MemberBegin(); tMember != tDocument.MemberEnd(); ++tMember)
    {
        std::string tKey = tMember->name.GetString();

        if (!tMember->value.IsString())
        {
            pError = "non-string value for key " + tKey + " in " + pPath;
            return false;
        }

        if (!pTarget.emplace(tKey, tMember->value.GetString()).second)
        {
            pError = "duplicate key " + tKey + " in " + pPath;
            return false;
        }
    }

    return true;
}

bool Localization::setLanguage(const std::string& pCode)
{
    auto tFound = mCatalogs.find(pCode);

    if (tFound == mCatalogs.end())
    {
        mLanguage = kBaseLanguage;
        mReported.clear();
        return false;
    }

    mLanguage = pCode;
    mReported.clear();

    if (mLanguage != kBaseLanguage)
    {
        size_t tMissing = 0;

        for (const auto& tEntry : getBase().texts)
        {
            if (tFound->second.texts.find(tEntry.first) == tFound->second.texts.end())
            {
                ++tMissing;
            }
        }

        SDL_Log("Localization: %s %zu keys, %zu missing", pCode.c_str(), tFound->second.texts.size(), tMissing);
    }

    return true;
}

const std::string& Localization::getLanguage() const
{
    return mLanguage;
}

std::vector<LanguageInfo> Localization::getAvailableLanguages() const
{
    std::vector<LanguageInfo> tResult;

    for (const auto& tEntry : mCatalogs)
    {
        tResult.push_back({ tEntry.first, tEntry.second.displayName });
    }

    std::sort(tResult.begin(), tResult.end(), [](const LanguageInfo& pLeft, const LanguageInfo& pRight)
    {
        bool tLeftBase = pLeft.code == kBaseLanguage;
        bool tRightBase = pRight.code == kBaseLanguage;

        if (tLeftBase || tRightBase)
        {
            return tLeftBase && !tRightBase;
        }

        return pLeft.code < pRight.code;
    });

    return tResult;
}

const Localization::Catalog& Localization::getBase() const
{
    return mCatalogs.at(kBaseLanguage);
}

const Localization::Catalog& Localization::getSelected() const
{
    auto tFound = mCatalogs.find(mLanguage);
    return tFound != mCatalogs.end() ? tFound->second : getBase();
}

const std::string* Localization::findTextIn(const Catalog& pCatalog, std::string_view pKey)
{
    auto tFound = pCatalog.texts.find(std::string(pKey));
    return tFound != pCatalog.texts.end() ? &tFound->second : nullptr;
}

const std::string* Localization::findText(std::string_view pKey) const
{
    if (const std::string* tSelected = findTextIn(getSelected(), pKey))
    {
        return tSelected;
    }

    return findTextIn(getBase(), pKey);
}

void Localization::reportOnce(const std::string& pMessage) const
{
    std::lock_guard<std::mutex> tLock(mReportedMutex);

    if (mReported.insert(pMessage).second)
    {
        SDL_Log("Localization: %s", pMessage.c_str());
    }
}

std::string Localization::text(std::string_view pKey) const
{
    if (const std::string* tText = findText(pKey))
    {
        return *tText;
    }

    reportOnce("missing key " + std::string(pKey));
    return std::string(pKey);
}

std::string Localization::expand(std::string_view pKey, const std::string& pTemplate, const ArgList& pArgs) const
{
    std::string tResult;
    tResult.reserve(pTemplate.size() + 16);

    size_t tPos = 0;

    while (tPos < pTemplate.size())
    {
        size_t tOpen = pTemplate.find('{', tPos);
        size_t tClose = tOpen == std::string::npos ? std::string::npos : pTemplate.find('}', tOpen);

        if (tClose == std::string::npos)
        {
            tResult.append(pTemplate, tPos, std::string::npos);
            break;
        }

        tResult.append(pTemplate, tPos, tOpen - tPos);

        std::string_view tName(pTemplate.data() + tOpen + 1, tClose - tOpen - 1);
        auto tArg = std::find_if(pArgs.begin(), pArgs.end(), [&](const auto& pEntry) { return pEntry.first == tName; });

        if (tArg != pArgs.end())
        {
            tResult += tArg->second;
        }
        else
        {
            reportOnce("missing argument {" + std::string(tName) + "} for " + std::string(pKey));
            tResult.append(pTemplate, tOpen, tClose - tOpen + 1);
        }

        tPos = tClose + 1;
    }

    return tResult;
}

std::string Localization::format(std::string_view pKey, TextArgs pArgs) const
{
    return expand(pKey, text(pKey), ArgList(pArgs.begin(), pArgs.end()));
}

std::string Localization::formatCount(std::string_view pKey, int64_t pCount, TextArgs pArgs) const
{
    std::string tOneKey = std::string(pKey) + ".one";
    const Catalog* tChain[2] = { &getSelected(), &getBase() };

    for (const Catalog* tCatalog : tChain)
    {
        const std::string* tBase = findTextIn(*tCatalog, pKey);

        if (!tBase)
        {
            continue;
        }

        const std::string* tOne = pCount == 1 ? findTextIn(*tCatalog, tOneKey) : nullptr;

        ArgList tArgs(pArgs.begin(), pArgs.end());
        tArgs.emplace_back("count", std::to_string(pCount));

        return expand(pKey, tOne ? *tOne : *tBase, tArgs);
    }

    reportOnce("missing key " + std::string(pKey));
    return std::string(pKey);
}

std::string Localization::content(std::string_view pKey, std::string_view pEnglish) const
{
    std::string tKey(pKey);
    const Catalog* tChain[2] = { &getSelected(), &getBase() };

    for (const Catalog* tCatalog : tChain)
    {
        auto tFound = tCatalog->contents.find(tKey);

        if (tFound != tCatalog->contents.end())
        {
            return tFound->second;
        }
    }

    return std::string(pEnglish);
}

std::string Localization::formatDecimal(double pValue, int pDigits) const
{
    char tBuffer[64];
    std::snprintf(tBuffer, sizeof(tBuffer), "%.*f", pDigits, pValue);

    std::string tResult = tBuffer;
    const std::string& tSeparator = getSelected().decimalSeparator;

    if (tSeparator != ".")
    {
        size_t tDot = tResult.find('.');

        if (tDot != std::string::npos)
        {
            tResult.replace(tDot, 1, tSeparator);
        }
    }

    return tResult;
}

void Localization::translateMarkup(std::string& pOut, const std::string& pIn) const
{
    if (pIn.find("[[") == std::string::npos)
    {
        pOut = pIn;
        return;
    }

    std::string tResult;
    size_t tPos = 0;

    while (tPos < pIn.size())
    {
        size_t tOpen = pIn.find("[[", tPos);

        if (tOpen == std::string::npos)
        {
            tResult.append(pIn, tPos, std::string::npos);
            break;
        }

        size_t tClose = pIn.find("]]", tOpen + 2);
        bool tIsToken = tClose != std::string::npos && tClose > tOpen + 2;

        for (size_t tIndex = tOpen + 2; tIsToken && tIndex < tClose; ++tIndex)
        {
            tIsToken = isKeyCharacter(pIn[tIndex]);
        }

        if (!tIsToken)
        {
            tResult.append(pIn, tPos, tOpen + 2 - tPos);
            tPos = tOpen + 2;
            continue;
        }

        tResult.append(pIn, tPos, tOpen - tPos);

        std::string tKey = pIn.substr(tOpen + 2, tClose - tOpen - 2);

        if (const std::string* tText = findText(tKey))
        {
            tResult += *tText;
        }
        else
        {
            reportOnce("missing key " + tKey);
            tResult.append(pIn, tOpen, tClose + 2 - tOpen);
        }

        tPos = tClose + 2;
    }

    pOut = std::move(tResult);
}

std::string Localization::escapeMarkup(const std::string& pText)
{
    std::string tResult;
    tResult.reserve(pText.size());

    for (size_t tIndex = 0; tIndex < pText.size(); ++tIndex)
    {
        tResult += pText[tIndex];

        if (pText[tIndex] == '[' && tIndex + 1 < pText.size() && pText[tIndex + 1] == '[')
        {
            tResult += kWordJoiner;
        }
    }

    return tResult;
}
