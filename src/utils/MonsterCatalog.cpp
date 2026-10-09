#include "MonsterCatalog.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <rapidjson/document.h>

using namespace lakot;

namespace
{
    float getFloat(const rapidjson::Value& pObject, const char* pName, float pDefault)
    {
        auto tMember = pObject.FindMember(pName);
        return (tMember != pObject.MemberEnd() && tMember->value.IsNumber()) ? tMember->value.GetFloat() : pDefault;
    }

    int32_t getInt(const rapidjson::Value& pObject, const char* pName, int32_t pDefault)
    {
        auto tMember = pObject.FindMember(pName);
        return (tMember != pObject.MemberEnd() && tMember->value.IsInt()) ? tMember->value.GetInt() : pDefault;
    }

    std::string getString(const rapidjson::Value& pObject, const char* pName)
    {
        auto tMember = pObject.FindMember(pName);
        return (tMember != pObject.MemberEnd() && tMember->value.IsString()) ? tMember->value.GetString() : std::string();
    }
}

bool MonsterCatalog::load(const std::string& pDataDirectory, std::string& pError)
{
    std::filesystem::path tPath = std::filesystem::path(pDataDirectory) / "monsters.json";
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

    if (tDocument.HasParseError() || !tDocument.IsObject() || !tDocument.HasMember("monsters") || !tDocument["monsters"].IsArray())
    {
        pError = "monsters.json needs a monsters array";
        return false;
    }

    for (const auto& tEntry : tDocument["monsters"].GetArray())
    {
        MonsterTemplate tTemplate;
        tTemplate.id = static_cast<uint32_t>(getInt(tEntry, "id", 0));
        tTemplate.name = getString(tEntry, "name");
        tTemplate.model = getString(tEntry, "model");
        tTemplate.level = static_cast<uint32_t>(getInt(tEntry, "level", 1));
        tTemplate.minLevel = static_cast<uint32_t>(std::max(1, getInt(tEntry, "minLevel", static_cast<int32_t>(tTemplate.level))));
        tTemplate.maxLevel = std::max(tTemplate.minLevel, static_cast<uint32_t>(std::max(1, getInt(tEntry, "maxLevel", static_cast<int32_t>(tTemplate.level)))));
        tTemplate.scalePerLevel = std::max(0.0f, getFloat(tEntry, "scalePerLevel", 0.1f));
        tTemplate.maxHealth = getInt(tEntry, "maxHealth", 1);
        tTemplate.attack = getInt(tEntry, "attack", 1);
        tTemplate.defense = getInt(tEntry, "defense", 0);
        tTemplate.experience = static_cast<uint32_t>(getInt(tEntry, "experience", 0));
        tTemplate.moveSpeed = getFloat(tEntry, "moveSpeed", 3.0f);
        tTemplate.aggroRange = getFloat(tEntry, "aggroRange", 8.0f);
        tTemplate.attackRange = getFloat(tEntry, "attackRange", 1.8f);
        tTemplate.attackInterval = getFloat(tEntry, "attackInterval", 1.5f);
        tTemplate.leashRange = getFloat(tEntry, "leashRange", 30.0f);
        tTemplate.respawnSeconds = getFloat(tEntry, "respawnSeconds", 15.0f);
        auto tAggressive = tEntry.FindMember("aggressive");
        tTemplate.isAggressive = tAggressive == tEntry.MemberEnd() || !tAggressive->value.IsBool() || tAggressive->value.GetBool();

        tTemplate.projectile = getString(tEntry, "projectile");
        tTemplate.assistRange = std::max(0.0f, getFloat(tEntry, "assistRange", 0.0f));
        auto tBoss = tEntry.FindMember("boss");
        tTemplate.isBoss = tBoss != tEntry.MemberEnd() && tBoss->value.IsBool() && tBoss->value.GetBool();
        tTemplate.lootRolls = static_cast<uint32_t>(std::max(1, getInt(tEntry, "lootRolls", 1)));
        tTemplate.scale = std::max(0.1f, getFloat(tEntry, "scale", 1.0f));

        auto tTint = tEntry.FindMember("tint");

        if (tTint != tEntry.MemberEnd() && tTint->value.IsArray() && tTint->value.Size() >= 3)
        {
            for (rapidjson::SizeType tIndex = 0; tIndex < 3; ++tIndex)
            {
                if (tTint->value[tIndex].IsNumber())
                {
                    tTemplate.tint[tIndex] = std::max(0.0f, tTint->value[tIndex].GetFloat());
                }
            }
        }

        auto tGold = tEntry.FindMember("gold");

        if (tGold != tEntry.MemberEnd() && tGold->value.IsObject())
        {
            tTemplate.goldChance = std::clamp(getFloat(tGold->value, "chance", 0.0f), 0.0f, 1.0f);
            tTemplate.goldMin = static_cast<uint32_t>(std::max(0, getInt(tGold->value, "min", 0)));
            tTemplate.goldMax = std::max(tTemplate.goldMin, static_cast<uint32_t>(std::max(0, getInt(tGold->value, "max", 0))));
        }

        auto tDrops = tEntry.FindMember("drops");

        if (tDrops != tEntry.MemberEnd() && tDrops->value.IsArray())
        {
            for (const auto& tDropEntry : tDrops->value.GetArray())
            {
                if (!tDropEntry.IsObject())
                {
                    continue;
                }

                MonsterDrop tDrop;
                tDrop.itemId = static_cast<uint32_t>(std::max(0, getInt(tDropEntry, "itemId", 0)));
                tDrop.chance = std::clamp(getFloat(tDropEntry, "chance", 0.0f), 0.0f, 1.0f);
                tDrop.minCount = static_cast<uint32_t>(std::max(1, getInt(tDropEntry, "min", 1)));
                tDrop.maxCount = std::max(tDrop.minCount, static_cast<uint32_t>(std::max(1, getInt(tDropEntry, "max", 1))));

                auto tGuaranteed = tDropEntry.FindMember("guaranteed");
                tDrop.guaranteed = tGuaranteed != tDropEntry.MemberEnd() && tGuaranteed->value.IsBool() && tGuaranteed->value.GetBool();

                if (tDrop.itemId != 0)
                {
                    tTemplate.drops.push_back(tDrop);
                }
            }
        }

        if (tTemplate.id == 0 || tTemplate.maxHealth <= 0)
        {
            pError = "monsters.json has an entry without an id or with no health";
            return false;
        }

        mTemplates[tTemplate.id] = std::move(tTemplate);
    }

    return true;
}

const std::unordered_map<uint32_t, MonsterTemplate>& MonsterCatalog::getAll() const
{
    return mTemplates;
}

const MonsterTemplate* MonsterCatalog::find(uint32_t pId) const
{
    auto tIterator = mTemplates.find(pId);
    return tIterator == mTemplates.end() ? nullptr : &tIterator->second;
}

size_t MonsterCatalog::getCount() const
{
    return mTemplates.size();
}
