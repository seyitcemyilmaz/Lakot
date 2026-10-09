#include "CharacterRepository.h"
#include "MovementRules.h"

#include <iostream>
#include <syncstream>

#include "../../network/RequestLimits.h"

using namespace lakot;

namespace
{
    // The column list every position query selects. Kept as one constant so a
    // new column cannot be added to one query and forgotten in another.
    constexpr const char* kPositionColumns = "id, name, map_id, pos_x, pos_y, pos_z, yaw";
    constexpr const char* kStatColumns =
        "level, experience, health, mana, strength, dexterity, intelligence, vitality, gold";

    lakot::CharacterRepository::CharacterRecord toRecord(const pqxx::row& pRow)
    {
        lakot::CharacterRepository::CharacterRecord tRecord;
        tRecord.characterId = pRow["id"].as<uint64_t>();
        tRecord.name = pRow["name"].as<std::string>();
        tRecord.mapId = pRow["map_id"].as<uint32_t>();
        tRecord.x = pRow["pos_x"].as<float>();
        tRecord.y = pRow["pos_y"].as<float>();
        tRecord.z = pRow["pos_z"].as<float>();
        tRecord.yaw = pRow["yaw"].as<float>();
        return tRecord;
    }

    void readStats(const pqxx::row& pRow, lakot::CharacterStats& pStats)
    {
        pStats.level = pRow["level"].as<uint32_t>();
        pStats.experience = pRow["experience"].as<uint64_t>();
        pStats.health = pRow["health"].as<int32_t>();
        pStats.mana = pRow["mana"].as<int32_t>();
        pStats.strength = pRow["strength"].as<int32_t>();
        pStats.dexterity = pRow["dexterity"].as<int32_t>();
        pStats.intelligence = pRow["intelligence"].as<int32_t>();
        pStats.vitality = pRow["vitality"].as<int32_t>();
        pStats.gold = pRow["gold"].as<uint64_t>();
    }
}

CharacterRepository::~CharacterRepository()
{

}

CharacterRepository::CharacterRepository(DatabaseManager& pDatabaseManager)
    : BaseRepository(pDatabaseManager)
{
}

void CharacterRepository::initializeTable()
{
    mDatabaseManager.executeAsync([](pqxx::connection& pConnection)
    {
        pqxx::work tWork(pConnection);

        tWork.exec(R"(
            CREATE TABLE IF NOT EXISTS characters (
                id BIGSERIAL PRIMARY KEY,
                account_id BIGINT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
                name VARCHAR(50) UNIQUE NOT NULL,
                map_id INTEGER NOT NULL DEFAULT 0,
                pos_x REAL NOT NULL DEFAULT 0,
                pos_y REAL NOT NULL DEFAULT 2,
                pos_z REAL NOT NULL DEFAULT 10,
                yaw REAL NOT NULL DEFAULT 0,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
        )");

        // Every listing is "this account's characters", so the foreign key
        // carries the only access pattern there is.
        tWork.exec("CREATE INDEX IF NOT EXISTS idx_characters_account ON characters (account_id);");

        // Name uniqueness is case-insensitive: two players called "Bob" and
        // "bob" would be indistinguishable in chat and whisper targeting,
        // which resolves by name. The UNIQUE constraint above is exact-match
        // only, so this is what actually enforces it.
        tWork.exec("CREATE UNIQUE INDEX IF NOT EXISTS idx_characters_name_lower ON characters (LOWER(name));");

        // Base stats only - every derived value (maximum health, attack,
        // defence) is computed from these by StatFormula and deliberately not
        // stored, so a balance change never needs a migration. health and mana
        // ARE stored because current vitals are state, not a derivation.
        // ADD COLUMN IF NOT EXISTS keeps this idempotent and lets the columns
        // land on an existing characters table without dropping it.
        tWork.exec(R"(
            ALTER TABLE characters
                ADD COLUMN IF NOT EXISTS level INTEGER NOT NULL DEFAULT 1,
                ADD COLUMN IF NOT EXISTS experience BIGINT NOT NULL DEFAULT 0,
                ADD COLUMN IF NOT EXISTS health INTEGER NOT NULL DEFAULT 0,
                ADD COLUMN IF NOT EXISTS mana INTEGER NOT NULL DEFAULT 0,
                ADD COLUMN IF NOT EXISTS strength INTEGER NOT NULL DEFAULT 10,
                ADD COLUMN IF NOT EXISTS dexterity INTEGER NOT NULL DEFAULT 10,
                ADD COLUMN IF NOT EXISTS intelligence INTEGER NOT NULL DEFAULT 10,
                ADD COLUMN IF NOT EXISTS vitality INTEGER NOT NULL DEFAULT 10,
                ADD COLUMN IF NOT EXISTS gold BIGINT NOT NULL DEFAULT 0;
        )");

        // Accounts that already had characters before kingdoms existed.
        tWork.exec(
            "UPDATE accounts SET kingdom = 1 "
            "WHERE kingdom IS NULL AND EXISTS (SELECT 1 FROM characters c WHERE c.account_id = accounts.id);");

        tWork.commit();

        std::osyncstream(std::cout) << "[CharacterRepo] Tablo hazir/kontrol edildi." << std::endl;
    }, kSchemaAffinityKey);
}

void CharacterRepository::listByAccount(uint64_t pAccountId, ListCallback pCallback)
{
    mDatabaseManager.executeAsync([pAccountId, pCallback](pqxx::connection& pConnection)
    {
        std::vector<CharacterRecord> tCharacters;
        uint32_t tKingdom = 0;

        try
        {
            pqxx::nontransaction tNtx(pConnection);

            auto tAccountResult = tNtx.exec("SELECT COALESCE(kingdom, 0) AS kingdom FROM accounts WHERE id = $1",
                                            pqxx::params{pAccountId});

            if (!tAccountResult.empty())
            {
                tKingdom = tAccountResult[0]["kingdom"].as<uint32_t>();
            }

            auto tResult = tNtx.exec(
                "SELECT id, name, map_id, pos_x, pos_y, pos_z, yaw FROM characters "
                "WHERE account_id = $1 ORDER BY created_at ASC",
                pqxx::params{pAccountId});

            for (const auto& tRow : tResult)
            {
                tCharacters.push_back(toRecord(tRow));
            }
        }
        catch (const std::exception& tException)
        {
            std::osyncstream(std::cerr) << "[CharacterRepo] Liste hatasi: " << tException.what() << std::endl;
        }

        if (pCallback)
        {
            pCallback(std::move(tCharacters), tKingdom);
        }
    }, pAccountId);
}

void CharacterRepository::createCharacter(uint64_t pAccountId, const std::string& pName, uint32_t pRequestedKingdom,
                                          const MapCatalog& pMapCatalog, CreateCallback pCallback)
{
    // Same worker for the same account, so a client firing two create
    // requests at once cannot pass the slot check twice before either insert
    // lands - they serialize on one connection instead of racing.
    mDatabaseManager.executeAsync([pAccountId, pName, pRequestedKingdom, &pMapCatalog, pCallback](pqxx::connection& pConnection)
    {
        CharacterRecord tRecord;

        try
        {
            pqxx::work tWork(pConnection);

            // Locked so the kingdom is decided exactly once even if two first
            // characters race.
            auto tAccountResult = tWork.exec("SELECT COALESCE(kingdom, 0) AS kingdom FROM accounts WHERE id = $1 FOR UPDATE",
                                             pqxx::params{pAccountId});

            if (tAccountResult.empty())
            {
                pCallback(CreateResult::eDatabaseError, tRecord);
                return;
            }

            uint32_t tKingdom = tAccountResult[0]["kingdom"].as<uint32_t>();

            if (tKingdom == 0)
            {
                if (!MapCatalog::isValidKingdom(pRequestedKingdom))
                {
                    pCallback(CreateResult::eInvalidKingdom, tRecord);
                    return;
                }

                tKingdom = pRequestedKingdom;

                // Commits together with the character below, or not at all.
                tWork.exec("UPDATE accounts SET kingdom = $1 WHERE id = $2",
                           pqxx::params{static_cast<int>(tKingdom), pAccountId});
            }

            auto tCountResult = tWork.exec("SELECT COUNT(*) AS c FROM characters WHERE account_id = $1",
                                           pqxx::params{pAccountId});

            if (tCountResult[0]["c"].as<uint32_t>() >= kMaxCharactersPerAccount)
            {
                pCallback(CreateResult::eNoFreeSlot, tRecord);
                return;
            }

            auto tNameResult = tWork.exec("SELECT 1 FROM characters WHERE LOWER(name) = LOWER($1)",
                                          pqxx::params{pName});

            if (!tNameResult.empty())
            {
                pCallback(CreateResult::eNameInUse, tRecord);
                return;
            }

            // Vitals are computed here rather than left to the column
            // defaults: the defaults cannot know the formula, and a character
            // that starts at 0 health would be indistinguishable from a
            // corrupt row.
            CharacterStats tNewStats;
            StatFormula::initializeVitals(tNewStats);

            // Explicit for the same reason: the start is a property of the
            // kingdom's map data, which the column defaults cannot know.
            const MapData* tStartMap = pMapCatalog.getStartMap(tKingdom);
            const MapSpawn& tSpawn = tStartMap->getSpawn();
            float tSpawnY = tStartMap->getHeightAt(tSpawn.x, tSpawn.z) + MovementRules::kBodyHalfHeight;

            auto tInsertResult = tWork.exec(
                "INSERT INTO characters (account_id, name, health, mana, map_id, pos_x, pos_y, pos_z, yaw) "
                "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) "
                "RETURNING " + std::string(kPositionColumns) + ", " + kStatColumns,
                pqxx::params{pAccountId, pName, tNewStats.health, tNewStats.mana,
                             static_cast<int64_t>(tStartMap->getId()), tSpawn.x, tSpawnY, tSpawn.z, tSpawn.yaw});

            tRecord = toRecord(tInsertResult[0]);
            readStats(tInsertResult[0], tRecord.stats);
            tRecord.kingdom = tKingdom;

            tWork.commit();

            pCallback(CreateResult::eCreated, tRecord);
        }
        catch (const pqxx::unique_violation&)
        {
            // The LOWER(name) unique index caught a name that slipped past
            // the check above (two accounts creating the same name at the
            // same instant, on different workers).
            pCallback(CreateResult::eNameInUse, tRecord);
        }
        catch (const std::exception& tException)
        {
            std::osyncstream(std::cerr) << "[CharacterRepo] Olusturma hatasi: " << tException.what() << std::endl;
            pCallback(CreateResult::eDatabaseError, tRecord);
        }
    }, pAccountId);
}

void CharacterRepository::findOwnedCharacter(uint64_t pAccountId, uint64_t pCharacterId, FindCallback pCallback)
{
    mDatabaseManager.executeAsync([pAccountId, pCharacterId, pCallback](pqxx::connection& pConnection)
    {
        bool tIsFound = false;
        CharacterRecord tRecord;

        try
        {
            pqxx::nontransaction tNtx(pConnection);

            // account_id is part of the WHERE clause, not checked afterwards:
            // asking for a character that is not yours simply returns nothing.
            auto tResult = tNtx.exec(
                std::string("SELECT c.id, c.name, c.map_id, c.pos_x, c.pos_y, c.pos_z, c.yaw, ") + kStatColumns +
                ", COALESCE(a.kingdom, 0) AS kingdom"
                " FROM characters c JOIN accounts a ON a.id = c.account_id"
                " WHERE c.id = $1 AND c.account_id = $2",
                pqxx::params{pCharacterId, pAccountId});

            if (!tResult.empty())
            {
                tRecord = toRecord(tResult[0]);
                readStats(tResult[0], tRecord.stats);
                tRecord.kingdom = tResult[0]["kingdom"].as<uint32_t>();
                tIsFound = true;
            }
        }
        catch (const std::exception& tException)
        {
            std::osyncstream(std::cerr) << "[CharacterRepo] Karakter bulma hatasi: " << tException.what() << std::endl;
        }

        if (pCallback)
        {
            pCallback(tIsFound, tRecord);
        }
    }, pAccountId);
}

void CharacterRepository::saveCharacterState(uint64_t pCharacterId, uint32_t pMapId,
                                             float pX, float pY, float pZ, float pYaw,
                                             const CharacterStats& pStats)
{
    mDatabaseManager.executeAsync([pCharacterId, pMapId, pX, pY, pZ, pYaw, pStats](pqxx::connection& pConnection)
    {
        try
        {
            pqxx::work tWork(pConnection);

            tWork.exec(
                "UPDATE characters SET map_id=$1, pos_x=$2, pos_y=$3, pos_z=$4, yaw=$5, "
                "level=$6, experience=$7, health=$8, mana=$9, "
                "strength=$10, dexterity=$11, intelligence=$12, vitality=$13, gold=$14 "
                "WHERE id=$15",
                pqxx::params{pMapId, pX, pY, pZ, pYaw,
                             pStats.level, pStats.experience, pStats.health, pStats.mana,
                             pStats.strength, pStats.dexterity, pStats.intelligence,
                             pStats.vitality, pStats.gold,
                             pCharacterId});

            tWork.commit();
        }
        catch (const std::exception& tException)
        {
            std::osyncstream(std::cerr) << "[CharacterRepo] Karakter kaydi hatasi: " << tException.what() << std::endl;
        }
    }, pCharacterId);
}
