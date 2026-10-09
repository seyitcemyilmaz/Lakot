#include "MapData.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <rapidjson/document.h>

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb_image.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

using namespace lakot;

namespace
{
    bool readFile(const std::filesystem::path& pPath, std::string& pContent)
    {
        std::ifstream tFile(pPath, std::ios::binary);

        if (!tFile)
        {
            return false;
        }

        std::ostringstream tBuffer;
        tBuffer << tFile.rdbuf();
        pContent = tBuffer.str();
        return true;
    }

    float getFloat(const rapidjson::Value& pObject, const char* pName, float pDefault)
    {
        auto tMember = pObject.FindMember(pName);
        return (tMember != pObject.MemberEnd() && tMember->value.IsNumber()) ? tMember->value.GetFloat() : pDefault;
    }

    uint32_t getUint(const rapidjson::Value& pObject, const char* pName, uint32_t pDefault)
    {
        auto tMember = pObject.FindMember(pName);
        return (tMember != pObject.MemberEnd() && tMember->value.IsUint()) ? tMember->value.GetUint() : pDefault;
    }

    std::string getString(const rapidjson::Value& pObject, const char* pName)
    {
        auto tMember = pObject.FindMember(pName);
        return (tMember != pObject.MemberEnd() && tMember->value.IsString()) ? tMember->value.GetString() : std::string();
    }

    bool getBool(const rapidjson::Value& pObject, const char* pName, bool pDefault)
    {
        auto tMember = pObject.FindMember(pName);
        return (tMember != pObject.MemberEnd() && tMember->value.IsBool()) ? tMember->value.GetBool() : pDefault;
    }
}

std::unique_ptr<MapData> MapData::load(const std::string& pDirectory, std::string& pError)
{
    std::filesystem::path tDirectory(pDirectory);

    std::string tJson;

    if (!readFile(tDirectory / "map.json", tJson))
    {
        pError = "cannot read " + (tDirectory / "map.json").string();
        return nullptr;
    }

    rapidjson::Document tDocument;
    tDocument.Parse(tJson.c_str());

    if (tDocument.HasParseError() || !tDocument.IsObject())
    {
        pError = "invalid JSON in " + (tDirectory / "map.json").string();
        return nullptr;
    }

    std::unique_ptr<MapData> tMap(new MapData());

    tMap->mId = getUint(tDocument, "id", 0);
    tMap->mName = getString(tDocument, "name");
    tMap->mKingdom = getUint(tDocument, "kingdom", 0);
    tMap->mWorldSize = getFloat(tDocument, "worldSize", 0.0f);
    tMap->mWaterLevel = getFloat(tDocument, "waterLevel", 0.0f);

    float tHeightMin = getFloat(tDocument, "heightMin", 0.0f);
    float tHeightMax = getFloat(tDocument, "heightMax", 1.0f);

    if (tMap->mId == 0 || tMap->mWorldSize <= 0.0f || tHeightMax <= tHeightMin)
    {
        pError = "map.json is missing id, worldSize or a valid height range";
        return nullptr;
    }

    auto tSpawn = tDocument.FindMember("spawn");

    if (tSpawn != tDocument.MemberEnd() && tSpawn->value.IsObject())
    {
        tMap->mSpawn.x = getFloat(tSpawn->value, "x", 0.0f);
        tMap->mSpawn.z = getFloat(tSpawn->value, "z", 0.0f);
        tMap->mSpawn.yaw = getFloat(tSpawn->value, "yaw", 0.0f);
    }

    auto tPortals = tDocument.FindMember("portals");

    if (tPortals != tDocument.MemberEnd() && tPortals->value.IsArray())
    {
        for (const auto& tEntry : tPortals->value.GetArray())
        {
            MapPortal tPortal;
            tPortal.triggerX = getFloat(tEntry, "x", 0.0f);
            tPortal.triggerZ = getFloat(tEntry, "z", 0.0f);
            tPortal.triggerRadius = getFloat(tEntry, "radius", 0.0f);
            tPortal.targetMapId = getUint(tEntry, "targetMap", 0);
            tPortal.targetX = getFloat(tEntry, "targetX", 0.0f);
            tPortal.targetZ = getFloat(tEntry, "targetZ", 0.0f);
            tPortal.targetYaw = getFloat(tEntry, "targetYaw", 0.0f);
            tMap->mPortals.push_back(tPortal);
        }
    }

    auto tObjects = tDocument.FindMember("objects");

    if (tObjects != tDocument.MemberEnd() && tObjects->value.IsArray())
    {
        tMap->mObjects.reserve(tObjects->value.Size());

        for (const auto& tEntry : tObjects->value.GetArray())
        {
            MapObject tObject;
            tObject.type = getString(tEntry, "type");
            tObject.model = getString(tEntry, "model");
            tObject.yaw = getFloat(tEntry, "yaw", 0.0f);
            tObject.scale = getFloat(tEntry, "scale", 1.0f);
            tObject.stretch = getFloat(tEntry, "stretch", 1.0f);
            tObject.x = getFloat(tEntry, "x", 0.0f);
            tObject.z = getFloat(tEntry, "z", 0.0f);
            tObject.sizeX = getFloat(tEntry, "sx", 1.0f);
            tObject.sizeY = getFloat(tEntry, "sy", 1.0f);
            tObject.sizeZ = getFloat(tEntry, "sz", 1.0f);
            tObject.elevation = getFloat(tEntry, "elevation", 0.0f);
            tObject.isBlocking = getBool(tEntry, "block", false);
            tObject.isHidden = getBool(tEntry, "hidden", false);

            auto tColor = tEntry.FindMember("color");

            if (tColor != tEntry.MemberEnd() && tColor->value.IsArray() && tColor->value.Size() == 3)
            {
                for (rapidjson::SizeType tIndex = 0; tIndex < 3; ++tIndex)
                {
                    tObject.color[tIndex] = tColor->value[tIndex].GetFloat();
                }
            }

            tMap->mObjects.push_back(std::move(tObject));
        }
    }

    auto tNpcs = tDocument.FindMember("npcs");

    if (tNpcs != tDocument.MemberEnd() && tNpcs->value.IsArray())
    {
        for (const auto& tEntry : tNpcs->value.GetArray())
        {
            MapNpc tNpc;
            tNpc.name = getString(tEntry, "name");
            tNpc.model = getString(tEntry, "model");
            tNpc.role = getString(tEntry, "role");
            tNpc.x = getFloat(tEntry, "x", 0.0f);
            tNpc.z = getFloat(tEntry, "z", 0.0f);
            tNpc.yaw = getFloat(tEntry, "yaw", 0.0f);
            tMap->mNpcs.push_back(std::move(tNpc));
        }
    }

    auto tMonsterSpawns = tDocument.FindMember("monsterSpawns");

    if (tMonsterSpawns != tDocument.MemberEnd() && tMonsterSpawns->value.IsArray())
    {
        for (const auto& tEntry : tMonsterSpawns->value.GetArray())
        {
            MapMonsterSpawn tSpawnPoint;
            tSpawnPoint.monsterId = getUint(tEntry, "monster", 0);
            tSpawnPoint.x = getFloat(tEntry, "x", 0.0f);
            tSpawnPoint.z = getFloat(tEntry, "z", 0.0f);
            tSpawnPoint.radius = getFloat(tEntry, "radius", 0.0f);
            tSpawnPoint.count = getUint(tEntry, "count", 1);
            tSpawnPoint.minLevel = getUint(tEntry, "minLevel", 0);
            tSpawnPoint.maxLevel = getUint(tEntry, "maxLevel", 0);
            tMap->mMonsterSpawns.push_back(tSpawnPoint);
        }
    }

    auto tSafeZones = tDocument.FindMember("safeZones");

    if (tSafeZones != tDocument.MemberEnd() && tSafeZones->value.IsArray())
    {
        for (const auto& tEntry : tSafeZones->value.GetArray())
        {
            MapSafeZone tZone;
            tZone.name = getString(tEntry, "name");
            auto tPoints = tEntry.FindMember("points");

            if (tPoints != tEntry.MemberEnd() && tPoints->value.IsArray())
            {
                for (const auto& tPoint : tPoints->value.GetArray())
                {
                    if (tPoint.IsArray() && tPoint.Size() == 2)
                    {
                        tZone.points.push_back({ tPoint[0].GetFloat(), tPoint[1].GetFloat() });
                    }
                }
            }

            if (tZone.points.size() >= 3)
            {
                tMap->mSafeZones.push_back(std::move(tZone));
            }
        }
    }

    // ---- Height ----

    std::string tHeightPath = (tDirectory / "height.png").string();
    int tWidth = 0;
    int tHeight = 0;
    int tChannels = 0;

    stbi_us* tHeightPixels = stbi_load_16(tHeightPath.c_str(), &tWidth, &tHeight, &tChannels, 1);

    if (!tHeightPixels || tWidth != tHeight || tWidth < 2)
    {
        stbi_image_free(tHeightPixels);
        pError = "cannot read a square height.png in " + tDirectory.string();
        return nullptr;
    }

    tMap->mResolution = tWidth;
    tMap->mGridSpacing = tMap->mWorldSize / static_cast<float>(tWidth - 1);
    tMap->mHeights.resize(static_cast<size_t>(tWidth) * tWidth);

    for (size_t tIndex = 0; tIndex < tMap->mHeights.size(); ++tIndex)
    {
        tMap->mHeights[tIndex] = tHeightMin + (tHeightPixels[tIndex] / 65535.0f) * (tHeightMax - tHeightMin);
    }

    stbi_image_free(tHeightPixels);

    // ---- Ground colour ----

    std::string tGroundPath = (tDirectory / "ground.png").string();
    stbi_uc* tGroundPixels = stbi_load(tGroundPath.c_str(), &tWidth, &tHeight, &tChannels, 3);

    if (!tGroundPixels || tWidth != tMap->mResolution || tHeight != tMap->mResolution)
    {
        stbi_image_free(tGroundPixels);
        pError = "ground.png in " + tDirectory.string() + " is missing or not the size of height.png";
        return nullptr;
    }

    tMap->mColors.assign(tGroundPixels, tGroundPixels + static_cast<size_t>(tWidth) * tWidth * 3);
    stbi_image_free(tGroundPixels);

    // ---- Block ----

    std::string tBlockPath = (tDirectory / "block.png").string();
    stbi_uc* tBlockPixels = stbi_load(tBlockPath.c_str(), &tWidth, &tHeight, &tChannels, 1);

    if (!tBlockPixels || tWidth != tHeight || tWidth < 1)
    {
        stbi_image_free(tBlockPixels);
        pError = "cannot read a square block.png in " + tDirectory.string();
        return nullptr;
    }

    tMap->mBlockResolution = tWidth;
    tMap->mBlocked.resize(static_cast<size_t>(tWidth) * tWidth);

    for (size_t tIndex = 0; tIndex < tMap->mBlocked.size(); ++tIndex)
    {
        tMap->mBlocked[tIndex] = tBlockPixels[tIndex] >= 128 ? 1 : 0;
    }

    stbi_image_free(tBlockPixels);

    // ---- Roads (optional) ----

    std::string tRoadsPath = (tDirectory / "roads.png").string();

    if (std::filesystem::exists(tRoadsPath))
    {
        stbi_uc* tRoadPixels = stbi_load(tRoadsPath.c_str(), &tWidth, &tHeight, &tChannels, 1);

        if (!tRoadPixels || tWidth != tMap->mBlockResolution || tHeight != tMap->mBlockResolution)
        {
            stbi_image_free(tRoadPixels);
            pError = "roads.png in " + tDirectory.string() + " is not the size of block.png";
            return nullptr;
        }

        tMap->mRoads.resize(tMap->mBlocked.size());

        for (size_t tIndex = 0; tIndex < tMap->mRoads.size(); ++tIndex)
        {
            tMap->mRoads[tIndex] = tRoadPixels[tIndex] >= 128 ? 1 : 0;
        }

        stbi_image_free(tRoadPixels);
    }

    // ---- Walk grid ----

    const int tCells = tMap->mBlockResolution;
    tMap->mCellWalkable.resize(static_cast<size_t>(tCells) * tCells);

    for (int tCellZ = 0; tCellZ < tCells; ++tCellZ)
    {
        for (int tCellX = 0; tCellX < tCells; ++tCellX)
        {
            bool tIsWalkable = tMap->isWalkable(tMap->getCellCenterX(tCellX), tMap->getCellCenterZ(tCellZ));
            tMap->mCellWalkable[static_cast<size_t>(tCellZ) * tCells + tCellX] = tIsWalkable ? 1 : 0;
        }
    }

    return tMap;
}

uint32_t MapData::getId() const
{
    return mId;
}

const std::string& MapData::getName() const
{
    return mName;
}

uint32_t MapData::getKingdom() const
{
    return mKingdom;
}

float MapData::getWorldSize() const
{
    return mWorldSize;
}

float MapData::getWaterLevel() const
{
    return mWaterLevel;
}

const MapSpawn& MapData::getSpawn() const
{
    return mSpawn;
}

const std::vector<MapPortal>& MapData::getPortals() const
{
    return mPortals;
}

const std::vector<MapNpc>& MapData::getNpcs() const
{
    return mNpcs;
}

const std::vector<MapMonsterSpawn>& MapData::getMonsterSpawns() const
{
    return mMonsterSpawns;
}

const std::vector<MapSafeZone>& MapData::getSafeZones() const
{
    return mSafeZones;
}

bool MapData::isSafe(float pX, float pZ) const
{
    for (const MapSafeZone& tZone : mSafeZones)
    {
        bool tIsInside = false;
        size_t tCount = tZone.points.size();

        for (size_t tIndex = 0, tPrevious = tCount - 1; tIndex < tCount; tPrevious = tIndex++)
        {
            const auto& tA = tZone.points[tIndex];
            const auto& tB = tZone.points[tPrevious];

            if ((tA[1] > pZ) != (tB[1] > pZ) && pX < (tB[0] - tA[0]) * (pZ - tA[1]) / (tB[1] - tA[1]) + tA[0])
            {
                tIsInside = !tIsInside;
            }
        }

        if (tIsInside)
        {
            return true;
        }
    }

    return false;
}

const std::vector<MapObject>& MapData::getObjects() const
{
    return mObjects;
}

int MapData::getResolution() const
{
    return mResolution;
}

float MapData::getGridSpacing() const
{
    return mGridSpacing;
}

float MapData::getVertexX(int pX) const
{
    return pX * mGridSpacing - mWorldSize * 0.5f;
}

float MapData::getVertexZ(int pZ) const
{
    return pZ * mGridSpacing - mWorldSize * 0.5f;
}

float MapData::getVertexHeight(int pX, int pZ) const
{
    return mHeights[static_cast<size_t>(pZ) * mResolution + pX];
}

std::array<uint8_t, 3> MapData::getVertexColor(int pX, int pZ) const
{
    size_t tIndex = (static_cast<size_t>(pZ) * mResolution + pX) * 3;
    return { mColors[tIndex], mColors[tIndex + 1], mColors[tIndex + 2] };
}

float MapData::getHeightAt(float pX, float pZ) const
{
    float tLast = static_cast<float>(mResolution - 1);
    float tGridX = std::clamp((pX + mWorldSize * 0.5f) / mGridSpacing, 0.0f, tLast);
    float tGridZ = std::clamp((pZ + mWorldSize * 0.5f) / mGridSpacing, 0.0f, tLast);

    int tCellX = std::min(static_cast<int>(tGridX), mResolution - 2);
    int tCellZ = std::min(static_cast<int>(tGridZ), mResolution - 2);

    float tU = tGridX - static_cast<float>(tCellX);
    float tV = tGridZ - static_cast<float>(tCellZ);

    float tTopLeft = getVertexHeight(tCellX, tCellZ);
    float tTopRight = getVertexHeight(tCellX + 1, tCellZ);
    float tBottomLeft = getVertexHeight(tCellX, tCellZ + 1);
    float tBottomRight = getVertexHeight(tCellX + 1, tCellZ + 1);

    // Same split as the terrain mesh: the diagonal runs top-right to
    // bottom-left.
    if (tU + tV <= 1.0f)
    {
        return tTopLeft + tU * (tTopRight - tTopLeft) + tV * (tBottomLeft - tTopLeft);
    }

    return tBottomRight + (1.0f - tU) * (tBottomLeft - tBottomRight) + (1.0f - tV) * (tTopRight - tBottomRight);
}

bool MapData::isInside(float pX, float pZ) const
{
    float tHalf = mWorldSize * 0.5f;
    return pX >= -tHalf && pX <= tHalf && pZ >= -tHalf && pZ <= tHalf;
}

bool MapData::isBlocked(float pX, float pZ) const
{
    float tCellSize = mWorldSize / static_cast<float>(mBlockResolution);
    int tCellX = static_cast<int>(std::floor((pX + mWorldSize * 0.5f) / tCellSize));
    int tCellZ = static_cast<int>(std::floor((pZ + mWorldSize * 0.5f) / tCellSize));

    if (tCellX < 0 || tCellZ < 0 || tCellX >= mBlockResolution || tCellZ >= mBlockResolution)
    {
        return true;
    }

    return mBlocked[static_cast<size_t>(tCellZ) * mBlockResolution + tCellX] != 0;
}

bool MapData::isWalkable(float pX, float pZ) const
{
    return isInside(pX, pZ) && getHeightAt(pX, pZ) >= mWaterLevel && !isBlocked(pX, pZ);
}

bool MapData::isMoveAllowed(float pFromX, float pFromZ, float pToX, float pToZ) const
{
    if (!isWalkable(pFromX, pFromZ))
    {
        return true;
    }

    // Finer than the smallest blocking feature (one 2-unit block cell), so
    // nothing solid can fall between two samples.
    return isPathWalkable(pFromX, pFromZ, pToX, pToZ, 1.0f);
}

bool MapData::isPathWalkable(float pFromX, float pFromZ, float pToX, float pToZ, float pSampleStep) const
{
    float tDx = pToX - pFromX;
    float tDz = pToZ - pFromZ;
    int tSteps = std::max(1, static_cast<int>(std::ceil(std::sqrt(tDx * tDx + tDz * tDz) / pSampleStep)));

    for (int tStep = 1; tStep <= tSteps; ++tStep)
    {
        float tT = static_cast<float>(tStep) / static_cast<float>(tSteps);

        if (!isWalkable(pFromX + tDx * tT, pFromZ + tDz * tT))
        {
            return false;
        }
    }

    return true;
}

int MapData::getCellResolution() const
{
    return mBlockResolution;
}

float MapData::getCellSize() const
{
    return mWorldSize / static_cast<float>(mBlockResolution);
}

int MapData::getCellIndex(float pCoordinate) const
{
    int tCell = static_cast<int>(std::floor((pCoordinate + mWorldSize * 0.5f) / getCellSize()));
    return std::clamp(tCell, 0, mBlockResolution - 1);
}

float MapData::getCellCenterX(int pCellX) const
{
    return (pCellX + 0.5f) * getCellSize() - mWorldSize * 0.5f;
}

float MapData::getCellCenterZ(int pCellZ) const
{
    return (pCellZ + 0.5f) * getCellSize() - mWorldSize * 0.5f;
}

bool MapData::isCellWalkable(int pCellX, int pCellZ) const
{
    if (pCellX < 0 || pCellZ < 0 || pCellX >= mBlockResolution || pCellZ >= mBlockResolution)
    {
        return false;
    }

    return mCellWalkable[static_cast<size_t>(pCellZ) * mBlockResolution + pCellX] != 0;
}

bool MapData::isCellRoad(int pCellX, int pCellZ) const
{
    if (mRoads.empty() || pCellX < 0 || pCellZ < 0 || pCellX >= mBlockResolution || pCellZ >= mBlockResolution)
    {
        return false;
    }

    return mRoads[static_cast<size_t>(pCellZ) * mBlockResolution + pCellX] != 0;
}
