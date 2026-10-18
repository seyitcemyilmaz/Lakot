#include "SkillCatalog.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include <rapidjson/document.h>

using namespace lakot;

namespace
{
    constexpr uint32_t kMaxSkillId = 65535;
    constexpr uint32_t kMaxSkillLevelLimit = 20;
    constexpr uint32_t kMaxPointsPerLevel = 5;
    constexpr float kMaxRange = 50.0f;

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

    bool readOptionalNumber(const rapidjson::Value& pObject, const char* pName, float& pOut)
    {
        auto tMember = pObject.FindMember(pName);

        if (tMember == pObject.MemberEnd())
        {
            return true;
        }

        if (!tMember->value.IsNumber())
        {
            return false;
        }

        pOut = tMember->value.GetFloat();
        return std::isfinite(pOut) && pOut >= 0.0f;
    }

    std::string readString(const rapidjson::Value& pObject, const char* pName)
    {
        auto tMember = pObject.FindMember(pName);
        return tMember != pObject.MemberEnd() && tMember->value.IsString() ? tMember->value.GetString() : std::string();
    }

    bool parseKind(const std::string& pText, SkillKindType& pOut)
    {
        if (pText == "damage") { pOut = SkillKindType::eDamage; return true; }
        if (pText == "heal") { pOut = SkillKindType::eHeal; return true; }
        if (pText == "buff") { pOut = SkillKindType::eBuff; return true; }
        if (pText == "debuff") { pOut = SkillKindType::eDebuff; return true; }
        if (pText == "movement") { pOut = SkillKindType::eMovement; return true; }
        if (pText == "summon") { pOut = SkillKindType::eSummon; return true; }
        return false;
    }

    bool parseTargeting(const std::string& pText, SkillTargetingType& pOut)
    {
        if (pText == "self") { pOut = SkillTargetingType::eSelf; return true; }
        if (pText == "single") { pOut = SkillTargetingType::eSingle; return true; }
        if (pText == "ally") { pOut = SkillTargetingType::eAlly; return true; }
        if (pText == "cone") { pOut = SkillTargetingType::eCone; return true; }
        if (pText == "area") { pOut = SkillTargetingType::eArea; return true; }
        return false;
    }

    bool readPath(const std::string& pPathKey, const ClassDefinition& pClass, uint32_t& pOut)
    {
        if (pPathKey == "fixed")
        {
            pOut = 0;
            return true;
        }

        for (const ClassPath& tPath : pClass.paths)
        {
            if (tPath.key == pPathKey)
            {
                pOut = tPath.id;
                return true;
            }
        }

        return false;
    }

    const ClassDefinition* findClassByKey(const ClassCatalog& pClassCatalog, const std::string& pKey)
    {
        for (const ClassDefinition& tClass : pClassCatalog.getAll())
        {
            if (tClass.key == pKey)
            {
                return &tClass;
            }
        }

        return nullptr;
    }

    bool readSkill(const rapidjson::Value& pEntry, const ClassCatalog& pClassCatalog, SkillDefinition& pSkill, std::string& pError)
    {
        if (!pEntry.IsObject())
        {
            pError = "skills.json has an entry that is not an object";
            return false;
        }

        pSkill.key = readString(pEntry, "key");
        pSkill.name = readString(pEntry, "name");
        pSkill.description = readString(pEntry, "description");

        if (!readUint(pEntry, "id", pSkill.id) || pSkill.id < 1 || pSkill.id > kMaxSkillId || pSkill.key.empty() || pSkill.name.empty())
        {
            pError = "skills.json has a skill without a valid id, key or name";
            return false;
        }

        const ClassDefinition* tClass = findClassByKey(pClassCatalog, readString(pEntry, "class"));

        if (!tClass)
        {
            pError = "skills.json skill has an unknown class: " + pSkill.key;
            return false;
        }

        pSkill.classId = tClass->id;

        if (!readPath(readString(pEntry, "path"), *tClass, pSkill.pathId))
        {
            pError = "skills.json skill has an unknown path: " + pSkill.key;
            return false;
        }

        if (!readUint(pEntry, "unlockLevel", pSkill.unlockLevel) || pSkill.unlockLevel < 1)
        {
            pError = "skills.json skill has an invalid unlockLevel: " + pSkill.key;
            return false;
        }

        if (!parseKind(readString(pEntry, "kind"), pSkill.kind) || !parseTargeting(readString(pEntry, "targeting"), pSkill.targeting))
        {
            pError = "skills.json skill has an unknown kind or targeting: " + pSkill.key;
            return false;
        }

        float tMpCost = 0.0f;

        if (!readOptionalNumber(pEntry, "range", pSkill.range) || pSkill.range > kMaxRange
            || !readOptionalNumber(pEntry, "cooldown", pSkill.cooldown)
            || !readOptionalNumber(pEntry, "mpCost", tMpCost))
        {
            pError = "skills.json skill has an invalid number: " + pSkill.key;
            return false;
        }

        pSkill.mpCost = static_cast<uint32_t>(tMpCost);
        return true;
    }

    bool validateClassLayout(const std::vector<SkillDefinition>& pSkills, const ClassCatalog& pClassCatalog, std::string& pError)
    {
        for (const ClassDefinition& tClass : pClassCatalog.getAll())
        {
            uint32_t tFixedCount = 0;

            for (const SkillDefinition& tSkill : pSkills)
            {
                if (tSkill.classId != tClass.id)
                {
                    continue;
                }

                if (tSkill.isFixed())
                {
                    ++tFixedCount;

                    if (tSkill.unlockLevel != 1)
                    {
                        pError = "skills.json fixed skill must unlock at level 1: " + tSkill.key;
                        return false;
                    }
                }
                else if (tSkill.unlockLevel < pClassCatalog.getPathUnlockLevel())
                {
                    pError = "skills.json path skill unlocks before the path choice: " + tSkill.key;
                    return false;
                }
            }

            if (tFixedCount != 1)
            {
                pError = "skills.json needs exactly one fixed skill for class: " + tClass.key;
                return false;
            }

            for (const ClassPath& tPath : tClass.paths)
            {
                bool tHasSkill = std::any_of(pSkills.begin(), pSkills.end(), [&](const SkillDefinition& pSkill)
                {
                    return pSkill.classId == tClass.id && pSkill.pathId == tPath.id;
                });

                if (!tHasSkill)
                {
                    pError = "skills.json path has no skills: " + tClass.key + "/" + tPath.key;
                    return false;
                }
            }
        }

        return true;
    }
}

bool SkillDefinition::isFixed() const
{
    return pathId == 0;
}

bool SkillCatalog::load(const std::string& pDataDirectory, const ClassCatalog& pClassCatalog, std::string& pError)
{
    std::filesystem::path tPath = std::filesystem::path(pDataDirectory) / "skills.json";
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

    if (tDocument.HasParseError() || !tDocument.IsObject() || !tDocument.HasMember("skills") || !tDocument["skills"].IsArray())
    {
        pError = "skills.json needs a skills array";
        return false;
    }

    uint32_t tMaxLevel = 0;

    if (!readUint(tDocument, "maxSkillLevel", tMaxLevel) || tMaxLevel < 1 || tMaxLevel > kMaxSkillLevelLimit)
    {
        pError = "skills.json maxSkillLevel must be 1.." + std::to_string(kMaxSkillLevelLimit);
        return false;
    }

    uint32_t tPerLevel = 0;
    auto tTiers = tDocument.FindMember("tiers");
    bool tHasNormal = tTiers != tDocument.MemberEnd() && tTiers->value.IsObject() && tTiers->value.HasMember("normal") && tTiers->value["normal"].IsObject();

    if (!tHasNormal || !readUint(tTiers->value["normal"], "pointsPerCharacterLevel", tPerLevel) || tPerLevel < 1 || tPerLevel > kMaxPointsPerLevel)
    {
        pError = "skills.json tiers.normal.pointsPerCharacterLevel must be 1.." + std::to_string(kMaxPointsPerLevel);
        return false;
    }

    std::vector<SkillDefinition> tSkills;
    std::unordered_set<uint32_t> tSeenIds;
    std::unordered_set<std::string> tSeenKeys;

    for (const auto& tEntry : tDocument["skills"].GetArray())
    {
        SkillDefinition tSkill;

        if (!readSkill(tEntry, pClassCatalog, tSkill, pError))
        {
            return false;
        }

        if (!tSeenIds.insert(tSkill.id).second || !tSeenKeys.insert(tSkill.key).second)
        {
            pError = "skills.json has a duplicate id or key: " + tSkill.key;
            return false;
        }

        tSkills.push_back(std::move(tSkill));
    }

    if (!validateClassLayout(tSkills, pClassCatalog, pError))
    {
        return false;
    }

    mSkills = std::move(tSkills);
    mMaxSkillLevel = tMaxLevel;
    mSkillPointsPerLevel = tPerLevel;
    return true;
}

const SkillDefinition* SkillCatalog::find(uint32_t pSkillId) const
{
    auto tIterator = std::find_if(mSkills.begin(), mSkills.end(), [pSkillId](const SkillDefinition& pSkill) { return pSkill.id == pSkillId; });
    return tIterator == mSkills.end() ? nullptr : &*tIterator;
}

std::vector<const SkillDefinition*> SkillCatalog::getForClass(uint32_t pClassId) const
{
    std::vector<const SkillDefinition*> tResult;

    for (const SkillDefinition& tSkill : mSkills)
    {
        if (tSkill.classId == pClassId)
        {
            tResult.push_back(&tSkill);
        }
    }

    std::stable_sort(tResult.begin(), tResult.end(), [](const SkillDefinition* pLeft, const SkillDefinition* pRight)
    {
        if (pLeft->pathId != pRight->pathId)
        {
            return pLeft->pathId < pRight->pathId;
        }

        return pLeft->unlockLevel < pRight->unlockLevel;
    });

    return tResult;
}

uint32_t SkillCatalog::getMaxSkillLevel() const
{
    return mMaxSkillLevel;
}

uint32_t SkillCatalog::getSkillPointsPerLevel() const
{
    return mSkillPointsPerLevel;
}
