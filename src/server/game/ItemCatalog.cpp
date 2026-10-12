#include "ItemCatalog.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>

#include <rapidjson/document.h>

using namespace lakot;

namespace
{
    constexpr int32_t kMaxRestoreAmount = 100000;
    constexpr uint32_t kMaxItemCells = 8;

    uint32_t getUint(const rapidjson::Value& pObject, const char* pName, uint32_t pDefault)
    {
        auto tMember = pObject.FindMember(pName);
        return tMember != pObject.MemberEnd() && tMember->value.IsUint() ? tMember->value.GetUint() : pDefault;
    }

    int32_t getInt(const rapidjson::Value& pObject, const char* pName)
    {
        auto tMember = pObject.FindMember(pName);
        return tMember != pObject.MemberEnd() && tMember->value.IsInt() ? tMember->value.GetInt() : 0;
    }

    std::string getString(const rapidjson::Value& pObject, const char* pName)
    {
        auto tMember = pObject.FindMember(pName);
        return tMember != pObject.MemberEnd() && tMember->value.IsString() ? tMember->value.GetString() : std::string();
    }
}

bool ItemCatalog::load(const std::string& pDataDirectory, std::string& pError)
{
    std::filesystem::path tPath = std::filesystem::path(pDataDirectory) / "items.json";
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

    if (tDocument.HasParseError() || !tDocument.IsObject() || !tDocument.HasMember("items") || !tDocument["items"].IsArray())
    {
        pError = "items.json needs an items array";
        return false;
    }

    std::vector<ItemTemplate> tTemplates;
    std::unordered_set<uint32_t> tSeenIds;

    for (const auto& tEntry : tDocument["items"].GetArray())
    {
        if (!tEntry.IsObject())
        {
            pError = "items.json has an entry that is not an object";
            return false;
        }

        ItemTemplate tTemplate;
        tTemplate.templateId = getUint(tEntry, "id", 0);
        tTemplate.name = getString(tEntry, "name");
        tTemplate.type = static_cast<ItemType>(getUint(tEntry, "type", 0));
        tTemplate.equipSlot = static_cast<EquipSlotType>(getUint(tEntry, "equipSlot", 0));
        tTemplate.maxStack = getUint(tEntry, "maxStack", 1);
        tTemplate.requiredLevel = getUint(tEntry, "requiredLevel", 1);
        tTemplate.bonusAttack = getInt(tEntry, "bonusAttack");
        tTemplate.bonusDefense = getInt(tEntry, "bonusDefense");
        tTemplate.bonusMaxHealth = getInt(tEntry, "bonusMaxHealth");
        tTemplate.bonusMaxMana = getInt(tEntry, "bonusMaxMana");
        tTemplate.restoreHealth = getInt(tEntry, "restoreHealth");
        tTemplate.restoreMana = getInt(tEntry, "restoreMana");
        tTemplate.width = getUint(tEntry, "width", 1);
        tTemplate.height = getUint(tEntry, "height", 1);
        tTemplate.icon = getString(tEntry, "icon");
        tTemplate.model = getString(tEntry, "model");
        tTemplate.tint = getUint(tEntry, "tint", 0xFFFFFF);

        if (tTemplate.templateId == 0 || tTemplate.name.empty())
        {
            pError = "items.json has an entry without an id or a name";
            return false;
        }

        if (tTemplate.restoreHealth < 0 || tTemplate.restoreHealth > kMaxRestoreAmount
            || tTemplate.restoreMana < 0 || tTemplate.restoreMana > kMaxRestoreAmount)
        {
            pError = "items.json has an entry with an out-of-range restore amount: " + tTemplate.name;
            return false;
        }

        if (tTemplate.type > ItemType::eQuest || tTemplate.equipSlot >= EquipSlotType::eCount)
        {
            pError = "items.json has an entry with an unknown type or equip slot: " + tTemplate.name;
            return false;
        }

        if (tTemplate.type == ItemType::eEquipment && tTemplate.maxStack != 1)
        {
            pError = "items.json has equipment that stacks: " + tTemplate.name;
            return false;
        }

        if (tTemplate.maxStack == 0 || tTemplate.width == 0 || tTemplate.height == 0
            || tTemplate.width > kMaxItemCells || tTemplate.height > kMaxItemCells)
        {
            pError = "items.json has an entry with a zero stack or an invalid size: " + tTemplate.name;
            return false;
        }

        if (!tSeenIds.insert(tTemplate.templateId).second)
        {
            pError = "items.json has a duplicate id: " + std::to_string(tTemplate.templateId);
            return false;
        }

        tTemplates.push_back(std::move(tTemplate));
    }

    setTemplates(std::move(tTemplates));
    return true;
}

void ItemCatalog::setTemplates(std::vector<ItemTemplate> pTemplates)
{
    std::sort(pTemplates.begin(), pTemplates.end(),
              [](const ItemTemplate& pLeft, const ItemTemplate& pRight)
              {
                  return pLeft.templateId < pRight.templateId;
              });

    mOrdered = std::move(pTemplates);

    mTemplates.clear();
    mTemplates.reserve(mOrdered.size());

    for (const ItemTemplate& tTemplate : mOrdered)
    {
        mTemplates.emplace(tTemplate.templateId, tTemplate);
    }
}

const std::vector<ItemTemplate>& ItemCatalog::getAll() const
{
    return mOrdered;
}

const ItemTemplate* ItemCatalog::find(uint32_t pTemplateId) const
{
    auto tIterator = mTemplates.find(pTemplateId);
    return tIterator == mTemplates.end() ? nullptr : &tIterator->second;
}

bool ItemCatalog::isEmpty() const
{
    return mTemplates.empty();
}

size_t ItemCatalog::size() const
{
    return mTemplates.size();
}
