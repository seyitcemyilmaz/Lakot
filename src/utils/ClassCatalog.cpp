#include "ClassCatalog.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include <rapidjson/document.h>

using namespace lakot;

namespace
{
    constexpr uint32_t kMaxStatPointsPerLevel = 20;
    constexpr uint32_t kMaxClassId = 255;
    constexpr uint32_t kMaxStatCap = 100000;

    bool readUint(const rapidjson::Value& pObject, const char* pName, uint32_t& pOut)
    {
        auto tMember = pObject.FindMember(pName);

        if (tMember == pObject.MemberEnd() || !tMember->value.IsUint())
        {
            return false;
        }

        pOut = tMember->value.GetUint();
        return true;
    }

    std::string readString(const rapidjson::Value& pObject, const char* pName)
    {
        auto tMember = pObject.FindMember(pName);
        return tMember != pObject.MemberEnd() && tMember->value.IsString() ? tMember->value.GetString() : std::string();
    }

    bool isMainStat(const std::string& pStat)
    {
        return pStat == "str" || pStat == "dex" || pStat == "int" || pStat == "vit";
    }

    bool readStartStat(const rapidjson::Value& pStats, const char* pName, uint32_t pCap, uint32_t& pOut)
    {
        return readUint(pStats, pName, pOut) && pOut >= 1 && pOut <= pCap;
    }

    bool readPaths(const rapidjson::Value& pEntry, ClassDefinition& pClass, std::string& pError)
    {
        auto tPaths = pEntry.FindMember("paths");

        if (tPaths == pEntry.MemberEnd() || !tPaths->value.IsArray() || tPaths->value.Empty())
        {
            pError = "classes.json class needs at least one path: " + pClass.key;
            return false;
        }

        for (const auto& tPathEntry : tPaths->value.GetArray())
        {
            ClassPath tPath;

            if (!tPathEntry.IsObject() || !readUint(tPathEntry, "id", tPath.id))
            {
                pError = "classes.json has a malformed path in class: " + pClass.key;
                return false;
            }

            tPath.key = readString(tPathEntry, "key");
            tPath.name = readString(tPathEntry, "name");

            if (tPath.key.empty() || tPath.name.empty())
            {
                pError = "classes.json has a path without a key or a name in class: " + pClass.key;
                return false;
            }

            if (tPath.id != pClass.paths.size() + 1)
            {
                pError = "classes.json path ids must count up from 1 in class: " + pClass.key;
                return false;
            }

            pClass.paths.push_back(std::move(tPath));
        }

        return true;
    }

    bool readClass(const rapidjson::Value& pEntry, uint32_t pCap, ClassDefinition& pClass, std::string& pError)
    {
        if (!pEntry.IsObject())
        {
            pError = "classes.json has a class that is not an object";
            return false;
        }

        pClass.key = readString(pEntry, "key");
        pClass.name = readString(pEntry, "name");

        if (!readUint(pEntry, "id", pClass.id) || pClass.id < 1 || pClass.id > kMaxClassId || pClass.key.empty() || pClass.name.empty())
        {
            pError = "classes.json has a class without a valid id, key or name";
            return false;
        }

        auto tMain = pEntry.FindMember("mainStats");

        if (tMain == pEntry.MemberEnd() || !tMain->value.IsArray() || tMain->value.Empty())
        {
            pError = "classes.json class needs mainStats: " + pClass.key;
            return false;
        }

        for (const auto& tStat : tMain->value.GetArray())
        {
            if (!tStat.IsString() || !isMainStat(tStat.GetString()))
            {
                pError = "classes.json class has an unknown main stat: " + pClass.key;
                return false;
            }

            pClass.mainStats.push_back(tStat.GetString());
        }

        auto tStart = pEntry.FindMember("startStats");

        if (tStart == pEntry.MemberEnd() || !tStart->value.IsObject()
            || !readStartStat(tStart->value, "str", pCap, pClass.startStrength)
            || !readStartStat(tStart->value, "dex", pCap, pClass.startDexterity)
            || !readStartStat(tStart->value, "int", pCap, pClass.startIntelligence)
            || !readStartStat(tStart->value, "vit", pCap, pClass.startVitality))
        {
            pError = "classes.json class has invalid startStats: " + pClass.key;
            return false;
        }

        return readPaths(pEntry, pClass, pError);
    }
}

const ClassPath* ClassDefinition::findPath(uint32_t pPathId) const
{
    auto tIterator = std::find_if(paths.begin(), paths.end(), [pPathId](const ClassPath& pPath) { return pPath.id == pPathId; });
    return tIterator == paths.end() ? nullptr : &*tIterator;
}

bool ClassCatalog::load(const std::string& pDataDirectory, std::string& pError)
{
    std::filesystem::path tPath = std::filesystem::path(pDataDirectory) / "classes.json";
    std::ifstream tFile(tPath, std::ios::binary);

    if (!tFile)
    {
        pError = "cannot read " + tPath.string();
        return false;
    }

    std::ostringstream tBuffer;
    tBuffer << tFile.rdbuf();
    std::string tJson = tBuffer.str();

    rapidjson::Document tDocument;
    tDocument.Parse(tJson.c_str());

    if (tDocument.HasParseError() || !tDocument.IsObject() || !tDocument.HasMember("classes") || !tDocument["classes"].IsArray())
    {
        pError = "classes.json needs a classes array";
        return false;
    }

    uint32_t tPerLevel = 0;
    uint32_t tCap = 0;
    uint32_t tUnlockLevel = 0;

    if (!readUint(tDocument, "statPointsPerLevel", tPerLevel) || tPerLevel < 1 || tPerLevel > kMaxStatPointsPerLevel)
    {
        pError = "classes.json statPointsPerLevel must be 1.." + std::to_string(kMaxStatPointsPerLevel);
        return false;
    }

    if (!readUint(tDocument, "statCap", tCap) || tCap < 1 || tCap > kMaxStatCap)
    {
        pError = "classes.json statCap must be 1.." + std::to_string(kMaxStatCap);
        return false;
    }

    if (!readUint(tDocument, "pathUnlockLevel", tUnlockLevel) || tUnlockLevel < 2)
    {
        pError = "classes.json pathUnlockLevel must be at least 2";
        return false;
    }

    std::vector<ClassDefinition> tClasses;
    std::unordered_set<uint32_t> tSeenIds;

    for (const auto& tEntry : tDocument["classes"].GetArray())
    {
        ClassDefinition tClass;

        if (!readClass(tEntry, tCap, tClass, pError))
        {
            return false;
        }

        if (!tSeenIds.insert(tClass.id).second)
        {
            pError = "classes.json has a duplicate class id: " + std::to_string(tClass.id);
            return false;
        }

        tClasses.push_back(std::move(tClass));
    }

    if (tClasses.empty())
    {
        pError = "classes.json has no classes";
        return false;
    }

    std::sort(tClasses.begin(), tClasses.end(), [](const ClassDefinition& pLeft, const ClassDefinition& pRight) { return pLeft.id < pRight.id; });

    mClasses = std::move(tClasses);
    mStatPointsPerLevel = tPerLevel;
    mStatCap = tCap;
    mPathUnlockLevel = tUnlockLevel;
    return true;
}

const ClassDefinition* ClassCatalog::find(uint32_t pClassId) const
{
    auto tIterator = std::find_if(mClasses.begin(), mClasses.end(), [pClassId](const ClassDefinition& pClass) { return pClass.id == pClassId; });
    return tIterator == mClasses.end() ? nullptr : &*tIterator;
}

const std::vector<ClassDefinition>& ClassCatalog::getAll() const
{
    return mClasses;
}

uint32_t ClassCatalog::getStatPointsPerLevel() const
{
    return mStatPointsPerLevel;
}

uint32_t ClassCatalog::getStatCap() const
{
    return mStatCap;
}

uint32_t ClassCatalog::getPathUnlockLevel() const
{
    return mPathUnlockLevel;
}
