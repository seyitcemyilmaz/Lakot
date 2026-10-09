#include "MapCatalog.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include <rapidjson/document.h>

using namespace lakot;

bool MapCatalog::isValidKingdom(uint32_t pKingdom)
{
    return pKingdom >= 1 && pKingdom <= kKingdomCount;
}

bool MapCatalog::load(const std::string& pDataDirectory, std::string& pError)
{
    namespace fs = std::filesystem;

    fs::path tDataDirectory(pDataDirectory);

    std::ifstream tFile(tDataDirectory / "kingdoms.json", std::ios::binary);

    if (!tFile)
    {
        pError = "cannot read " + (tDataDirectory / "kingdoms.json").string();
        return false;
    }

    std::ostringstream tBuffer;
    tBuffer << tFile.rdbuf();
    std::string tJson = tBuffer.str();

    rapidjson::Document tDocument;
    tDocument.Parse(tJson.c_str());

    if (tDocument.HasParseError() || !tDocument.IsObject() || !tDocument.HasMember("kingdoms")
        || !tDocument["kingdoms"].IsArray() || !tDocument.HasMember("defaultStartMap")
        || !tDocument["defaultStartMap"].IsUint())
    {
        pError = "kingdoms.json needs a kingdoms array and a defaultStartMap";
        return false;
    }

    mDefaultStartMapId = tDocument["defaultStartMap"].GetUint();

    for (const auto& tEntry : tDocument["kingdoms"].GetArray())
    {
        KingdomInfo tKingdom;
        tKingdom.id = tEntry.HasMember("id") && tEntry["id"].IsUint() ? tEntry["id"].GetUint() : 0;
        tKingdom.name = tEntry.HasMember("name") && tEntry["name"].IsString() ? tEntry["name"].GetString() : "";
        tKingdom.description = tEntry.HasMember("description") && tEntry["description"].IsString() ? tEntry["description"].GetString() : "";
        tKingdom.color = tEntry.HasMember("color") && tEntry["color"].IsString() ? tEntry["color"].GetString() : "#FFFFFF";
        tKingdom.startMapId = tEntry.HasMember("startMap") && tEntry["startMap"].IsUint() ? tEntry["startMap"].GetUint() : 0;

        if (!isValidKingdom(tKingdom.id))
        {
            pError = "kingdoms.json has a kingdom id outside 1.." + std::to_string(kKingdomCount);
            return false;
        }

        mKingdoms.push_back(std::move(tKingdom));
    }

    std::error_code tErrorCode;

    for (const auto& tEntry : fs::directory_iterator(tDataDirectory / "maps", tErrorCode))
    {
        if (!tEntry.is_directory() || !fs::exists(tEntry.path() / "map.json"))
        {
            continue;
        }

        std::unique_ptr<MapData> tMap = MapData::load(tEntry.path().string(), pError);

        if (!tMap)
        {
            return false;
        }

        uint32_t tId = tMap->getId();

        if (mMaps.count(tId) != 0)
        {
            pError = "two maps share id " + std::to_string(tId);
            return false;
        }

        mMaps.emplace(tId, std::move(tMap));
    }

    if (tErrorCode)
    {
        pError = "cannot list " + (tDataDirectory / "maps").string() + ": " + tErrorCode.message();
        return false;
    }

    if (!getMap(mDefaultStartMapId))
    {
        pError = "defaultStartMap " + std::to_string(mDefaultStartMapId) + " is not a loaded map";
        return false;
    }

    return true;
}

const MapData* MapCatalog::getMap(uint32_t pMapId) const
{
    auto tIterator = mMaps.find(pMapId);
    return tIterator == mMaps.end() ? nullptr : tIterator->second.get();
}

std::vector<uint32_t> MapCatalog::getMapIds() const
{
    std::vector<uint32_t> tIds;
    tIds.reserve(mMaps.size());

    for (const auto& [tId, tMap] : mMaps)
    {
        tIds.push_back(tId);
    }

    return tIds;
}

const std::vector<KingdomInfo>& MapCatalog::getKingdoms() const
{
    return mKingdoms;
}

const KingdomInfo* MapCatalog::getKingdom(uint32_t pKingdom) const
{
    for (const KingdomInfo& tKingdom : mKingdoms)
    {
        if (tKingdom.id == pKingdom)
        {
            return &tKingdom;
        }
    }

    return nullptr;
}

const MapData* MapCatalog::getStartMap(uint32_t pKingdom) const
{
    if (const KingdomInfo* tKingdom = getKingdom(pKingdom))
    {
        if (const MapData* tMap = getMap(tKingdom->startMapId))
        {
            return tMap;
        }
    }

    return getMap(mDefaultStartMapId);
}

std::optional<MapPortal> MapCatalog::findPortalAt(uint32_t pMapId, float pX, float pZ) const
{
    const MapData* tMap = getMap(pMapId);

    if (!tMap)
    {
        return std::nullopt;
    }

    for (const MapPortal& tPortal : tMap->getPortals())
    {
        float tDx = pX - tPortal.triggerX;
        float tDz = pZ - tPortal.triggerZ;

        if (tDx * tDx + tDz * tDz <= tPortal.triggerRadius * tPortal.triggerRadius)
        {
            return tPortal;
        }
    }

    return std::nullopt;
}
