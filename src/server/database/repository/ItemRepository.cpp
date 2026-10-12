#include "ItemRepository.h"

#include <iostream>
#include <syncstream>

using namespace lakot;

ItemRepository::~ItemRepository()
{

}

ItemRepository::ItemRepository(DatabaseManager& pDatabaseManager)
    : BaseRepository(pDatabaseManager)
{
}

void ItemRepository::initializeTable()
{
    mDatabaseManager.executeAsync([](pqxx::connection& pConnection)
    {
        pqxx::work tWork(pConnection);

        tWork.exec(R"(
            CREATE TABLE IF NOT EXISTS item_templates (
                id INTEGER PRIMARY KEY,
                name VARCHAR(80) NOT NULL,
                type INTEGER NOT NULL DEFAULT 0,
                equip_slot INTEGER NOT NULL DEFAULT 0,
                max_stack INTEGER NOT NULL DEFAULT 1,
                required_level INTEGER NOT NULL DEFAULT 1,
                bonus_attack INTEGER NOT NULL DEFAULT 0,
                bonus_defense INTEGER NOT NULL DEFAULT 0,
                bonus_max_health INTEGER NOT NULL DEFAULT 0,
                bonus_max_mana INTEGER NOT NULL DEFAULT 0
            );
        )");

        // Footprint in bag cells. Added after the table existed, so
        // ADD COLUMN IF NOT EXISTS rather than a new CREATE.
        tWork.exec(R"(
            ALTER TABLE item_templates
                ADD COLUMN IF NOT EXISTS width INTEGER NOT NULL DEFAULT 1,
                ADD COLUMN IF NOT EXISTS height INTEGER NOT NULL DEFAULT 1,
                ADD COLUMN IF NOT EXISTS icon VARCHAR(64) NOT NULL DEFAULT '',
                ADD COLUMN IF NOT EXISTS model VARCHAR(128) NOT NULL DEFAULT '',
                ADD COLUMN IF NOT EXISTS tint BIGINT NOT NULL DEFAULT 16777215;
        )");

        // ON DELETE CASCADE, so deleting a character cannot leave orphaned
        // items behind - the same treatment characters already get from
        // accounts.
        tWork.exec(R"(
            CREATE TABLE IF NOT EXISTS item_instances (
                id BIGSERIAL PRIMARY KEY,
                character_id BIGINT NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
                template_id INTEGER NOT NULL,
                count INTEGER NOT NULL DEFAULT 1,
                location INTEGER NOT NULL DEFAULT 0,
                slot INTEGER NOT NULL DEFAULT 0
            );
        )");

        tWork.exec("CREATE INDEX IF NOT EXISTS idx_item_instances_character ON item_instances (character_id);");

        tWork.commit();

        std::osyncstream(std::cout) << "[ItemRepo] Tablolar hazir/kontrol edildi." << std::endl;
    }, kSchemaAffinityKey);
}

void ItemRepository::syncTemplates(std::vector<ItemTemplate> pTemplates, SyncCallback pCallback)
{
    mDatabaseManager.executeAsync([pTemplates = std::move(pTemplates), pCallback](pqxx::connection& pConnection)
    {
        uint64_t tRemoved = 0;
        bool tIsSuccess = true;

        try
        {
            pqxx::work tWork(pConnection);
            std::string tIds;

            for (const ItemTemplate& tTemplate : pTemplates)
            {
                tWork.exec(
                    "INSERT INTO item_templates (id, name, type, equip_slot, max_stack, required_level, "
                    "bonus_attack, bonus_defense, bonus_max_health, bonus_max_mana, width, height, icon, model, tint) "
                    "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15) "
                    "ON CONFLICT (id) DO UPDATE SET name = EXCLUDED.name, type = EXCLUDED.type, "
                    "equip_slot = EXCLUDED.equip_slot, max_stack = EXCLUDED.max_stack, required_level = EXCLUDED.required_level, "
                    "bonus_attack = EXCLUDED.bonus_attack, bonus_defense = EXCLUDED.bonus_defense, "
                    "bonus_max_health = EXCLUDED.bonus_max_health, bonus_max_mana = EXCLUDED.bonus_max_mana, "
                    "width = EXCLUDED.width, height = EXCLUDED.height, icon = EXCLUDED.icon, model = EXCLUDED.model, tint = EXCLUDED.tint",
                    pqxx::params{ tTemplate.templateId, tTemplate.name, static_cast<uint32_t>(tTemplate.type),
                                  static_cast<uint32_t>(tTemplate.equipSlot), tTemplate.maxStack, tTemplate.requiredLevel,
                                  tTemplate.bonusAttack, tTemplate.bonusDefense, tTemplate.bonusMaxHealth, tTemplate.bonusMaxMana,
                                  tTemplate.width, tTemplate.height, tTemplate.icon, tTemplate.model, static_cast<int64_t>(tTemplate.tint) });

                tIds += (tIds.empty() ? "" : ",") + std::to_string(tTemplate.templateId);
            }

            if (!tIds.empty())
            {
                tRemoved = tWork.exec("DELETE FROM item_instances WHERE template_id NOT IN (" + tIds + ")").affected_rows();
                tWork.exec("DELETE FROM item_templates WHERE id NOT IN (" + tIds + ")");
            }

            tWork.commit();
        }
        catch (const std::exception& tException)
        {
            tIsSuccess = false;
            std::osyncstream(std::cerr) << "[ItemRepo] Sablon esitleme hatasi: " << tException.what() << std::endl;
        }

        if (pCallback)
        {
            pCallback(tIsSuccess, tRemoved);
        }
    }, kSchemaAffinityKey);
}

std::vector<ItemInstance> ItemRepository::readItems(pqxx::transaction_base& pTransaction, uint64_t pCharacterId)
{
    std::vector<ItemInstance> tItems;

    auto tResult = pTransaction.exec(
        "SELECT id, template_id, count, location, slot FROM item_instances "
        "WHERE character_id = $1",
        pqxx::params{pCharacterId});

    tItems.reserve(tResult.size());

    for (const auto& tRow : tResult)
    {
        ItemInstance tItem;
        tItem.itemId = tRow["id"].as<uint64_t>();
        tItem.templateId = tRow["template_id"].as<uint32_t>();
        tItem.count = tRow["count"].as<uint32_t>();
        tItem.location = static_cast<ItemLocationType>(tRow["location"].as<uint32_t>());
        tItem.slot = tRow["slot"].as<uint32_t>();

        tItems.push_back(tItem);
    }

    return tItems;
}

void ItemRepository::writeItems(pqxx::work& pWork, uint64_t pCharacterId, const std::vector<ItemInstance>& pItems)
{
    pWork.exec("DELETE FROM item_instances WHERE character_id = $1", pqxx::params{pCharacterId});

    if (pItems.empty())
    {
        return;
    }

    std::vector<int32_t> tTemplateIds;
    std::vector<int32_t> tCounts;
    std::vector<int32_t> tLocations;
    std::vector<int32_t> tSlots;

    tTemplateIds.reserve(pItems.size());
    tCounts.reserve(pItems.size());
    tLocations.reserve(pItems.size());
    tSlots.reserve(pItems.size());

    for (const ItemInstance& tItem : pItems)
    {
        tTemplateIds.push_back(static_cast<int32_t>(tItem.templateId));
        tCounts.push_back(static_cast<int32_t>(tItem.count));
        tLocations.push_back(static_cast<int32_t>(tItem.location));
        tSlots.push_back(static_cast<int32_t>(tItem.slot));
    }

    pWork.exec(
        "INSERT INTO item_instances (character_id, template_id, count, location, slot) "
        "SELECT $1, t.template_id, t.count, t.location, t.slot "
        "FROM unnest($2::int[], $3::int[], $4::int[], $5::int[]) AS t(template_id, count, location, slot)",
        pqxx::params{pCharacterId, tTemplateIds, tCounts, tLocations, tSlots});
}
