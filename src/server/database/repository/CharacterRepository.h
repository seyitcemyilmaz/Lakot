#ifndef LAKOT_SERVER_CHARACTERREPOSITORY_H
#define LAKOT_SERVER_CHARACTERREPOSITORY_H

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "BaseRepository.h"

#include "MapCatalog.h"

#include "../../game/CharacterStats.h"
#include "../../game/Inventory.h"

namespace lakot
{

// Characters are what exist in the world: they own the position, the name
// other players see, and (later) inventory, stats and skills. An account owns
// several of them and is only an authentication identity. That split is why
// this is its own repository rather than more columns on accounts, where the
// position used to live.
class CharacterRepository : public BaseRepository
{
public:
    // Hard ceiling on characters per account, enforced server-side and sent
    // to the client rather than hardcoded there.
    static constexpr uint32_t kMaxCharactersPerAccount = 5;

    enum class CreateResult
    {
        eCreated,
        eNameInUse,
        eNameInvalid,
        eNoFreeSlot,
        eInvalidKingdom,
        eDatabaseError
    };

    struct CharacterRecord
    {
        uint64_t characterId = 0;
        std::string name;
        uint32_t mapId = 0;
        float x = 0.0f;
        float y = 2.0f;
        float z = 10.0f;
        float yaw = 0.0f;

        // The owning account's kingdom, 0 if none yet. Filled by
        // loadOwnedCharacter.
        uint32_t kingdom = 0;

        uint32_t gmLevel = 0;
        int64_t mutedUntilEpoch = 0;

        // Populated by loadOwnedCharacter (entering the world needs them) but
        // left at defaults by listByAccount, which only feeds the selection
        // screen and has no use for them.
        CharacterStats stats;
    };

    // pAccountKingdom is 0 while the account has not chosen one yet.
    using ListCallback = std::function<void(std::vector<CharacterRecord>, uint32_t pAccountKingdom)>;
    using CreateCallback = std::function<void(CreateResult, const CharacterRecord&)>;
    enum class LoadResult
    {
        eLoaded,
        eNotFound,
        eDatabaseError
    };

    struct CharacterSave
    {
        uint64_t characterId = 0;
        uint32_t mapId = 0;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yaw = 0.0f;
        CharacterStats stats;
        std::vector<ItemInstance> items;
    };

    using LoadCallback = std::function<void(LoadResult, const CharacterRecord&, std::vector<ItemInstance>)>;

    virtual ~CharacterRepository();
    explicit CharacterRepository(DatabaseManager& pDatabaseManager);

    void initializeTable() override;

    void listByAccount(uint64_t pAccountId, ListCallback pCallback);

    // The account's first character also fixes its kingdom to
    // pRequestedKingdom (which must then be valid); later characters inherit
    // the account's kingdom and ignore it. Every character starts at its
    // kingdom's starting map, taken from pMapCatalog.
    void createCharacter(uint64_t pAccountId, const std::string& pName, uint32_t pRequestedKingdom,
                         const MapCatalog& pMapCatalog, CreateCallback pCallback);

    // Looks the character up AND checks it belongs to pAccountId - the two
    // are one query on purpose, so no caller can accidentally load a
    // character without proving ownership of it.
    void loadOwnedCharacter(uint64_t pAccountId, uint64_t pCharacterId, LoadCallback pCallback);

    void saveCharacter(CharacterSave pSave);
};

}

#endif
