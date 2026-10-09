#ifndef LAKOT_SERVER_ITEMREPOSITORY_H
#define LAKOT_SERVER_ITEMREPOSITORY_H

#include <cstdint>
#include <functional>
#include <vector>

#include "BaseRepository.h"

#include "../../game/Inventory.h"
#include "../../game/ItemTemplate.h"

namespace lakot
{

class ItemRepository : public BaseRepository
{
public:
    using SyncCallback = std::function<void(bool pIsSuccess, uint64_t pRemovedInstances)>;
    using ItemLoadCallback = std::function<void(std::vector<ItemInstance>)>;

    virtual ~ItemRepository();
    explicit ItemRepository(DatabaseManager& pDatabaseManager);

    // Creates both tables and seeds the starter templates. Must be queued
    // after CharacterRepository's - item_instances references characters -
    // which BaseRepository::kSchemaAffinityKey guarantees by pinning every
    // schema statement to one connection in call order.
    void initializeTable() override;

    // Writes data/items.json's templates through and drops item instances whose template no longer exists.
    void syncTemplates(std::vector<ItemTemplate> pTemplates, SyncCallback pCallback);

    void loadCharacterItems(uint64_t pCharacterId, ItemLoadCallback pCallback);

    // Replaces the character's whole item set in one transaction.
    //
    // Deliberately delete-then-insert rather than a per-row diff: an
    // inventory is small, it is always loaded and saved as a whole by the one
    // zone thread that owns the character, and a diff would need change
    // tracking on every mutation for no benefit yet. The cost is that
    // ItemInstance::itemId is NOT stable across a save - nothing depends on
    // it today, but anything that later needs a durable per-item identity
    // (trade logs, mail attachments, item history) has to replace this with a
    // real diff first.
    void saveCharacterItems(uint64_t pCharacterId, std::vector<ItemInstance> pItems);
};

}

#endif
