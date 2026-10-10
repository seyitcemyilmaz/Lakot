#include "GmLogRepository.h"

#include <iostream>
#include <syncstream>

using namespace lakot;

GmLogRepository::GmLogRepository(DatabaseManager& pDatabaseManager)
    : BaseRepository(pDatabaseManager)
{

}

void GmLogRepository::initializeTable()
{
    mDatabaseManager.executeAsync([](pqxx::connection& pConnection)
    {
        pqxx::work tWork(pConnection);

        tWork.exec(R"(
            CREATE TABLE IF NOT EXISTS gm_log (
                id BIGSERIAL PRIMARY KEY,
                created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
                account_id BIGINT NOT NULL,
                character_id BIGINT NOT NULL,
                character_name TEXT NOT NULL,
                gm_level SMALLINT NOT NULL,
                command TEXT NOT NULL,
                arguments TEXT NOT NULL DEFAULT '',
                target TEXT NOT NULL DEFAULT '',
                result TEXT NOT NULL DEFAULT ''
            );
        )");

        tWork.exec("CREATE INDEX IF NOT EXISTS idx_gm_log_account ON gm_log (account_id, created_at);");

        tWork.commit();

        std::osyncstream(std::cout) << "[GmLogRepo] Tablo hazir/kontrol edildi." << std::endl;
    }, kSchemaAffinityKey);
}

void GmLogRepository::record(Entry pEntry)
{
    mDatabaseManager.executeAsync([pEntry = std::move(pEntry)](pqxx::connection& pConnection)
    {
        try
        {
            pqxx::work tWork(pConnection);

            tWork.exec(
                "INSERT INTO gm_log (account_id, character_id, character_name, gm_level, command, arguments, target, result) "
                "VALUES ($1, $2, $3, $4, $5, $6, $7, $8)",
                pqxx::params{pEntry.accountId, pEntry.characterId, pEntry.characterName,
                             static_cast<int>(pEntry.gmLevel), pEntry.command, pEntry.arguments,
                             pEntry.target, pEntry.result});

            tWork.commit();
        }
        catch (const std::exception& tException)
        {
            std::osyncstream(std::cerr) << "[GmLogRepo] Kayit hatasi: " << tException.what() << std::endl;
        }
    });
}
