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

void ItemRepository::loadCharacterItems(uint64_t pCharacterId, ItemLoadCallback pCallback)
{
    mDatabaseManager.executeAsync([pCharacterId, pCallback](pqxx::connection& pConnection)
    {
        std::vector<ItemInstance> tItems;

        try
        {
            pqxx::nontransaction tNtx(pConnection);

            auto tResult = tNtx.exec(
                "SELECT id, template_id, count, location, slot FROM item_instances "
                "WHERE character_id = $1",
                pqxx::params{pCharacterId});

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
        }
        catch (const std::exception& tException)
        {
            std::osyncstream(std::cerr) << "[ItemRepo] Esya yukleme hatasi: " << tException.what() << std::endl;
        }

        if (pCallback)
        {
            pCallback(std::move(tItems));
        }
    }, pCharacterId);
}

void ItemRepository::saveCharacterItems(uint64_t pCharacterId, std::vector<ItemInstance> pItems)
{
    // Same affinity key as loadCharacterItems, so one character's loads and
    // saves always run in submission order on a single connection and cannot
    // interleave with each other.
    mDatabaseManager.executeAsync([pCharacterId, pItems = std::move(pItems)](pqxx::connection& pConnection)
    {
        try
        {
            pqxx::work tWork(pConnection);

            tWork.exec("DELETE FROM item_instances WHERE character_id = $1", pqxx::params{pCharacterId});

            for (const ItemInstance& tItem : pItems)
            {
                tWork.exec(
                    "INSERT INTO item_instances (character_id, template_id, count, location, slot) "
                    "VALUES ($1, $2, $3, $4, $5)",
                    pqxx::params{pCharacterId, tItem.templateId, tItem.count,
                                 static_cast<uint32_t>(tItem.location), tItem.slot});
            }

            // The delete and every insert commit together, so a crash midway
            // cannot leave a character with half an inventory.
            tWork.commit();
        }
        catch (const std::exception& tException)
        {
            std::osyncstream(std::cerr) << "[ItemRepo] Esya kaydi hatasi: " << tException.what() << std::endl;
        }
    }, pCharacterId);
}
