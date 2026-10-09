#ifndef LAKOT_INVENTORYCONTROLLER_H
#define LAKOT_INVENTORYCONTROLLER_H

#include <cstdint>
#include <optional>
#include <functional>
#include <unordered_map>
#include <vector>

#include <connection.pb.h>

#include "../GameTypes.h"

namespace lakot
{

class NetworkManager;

template <typename MessageType>
class NetworkSession;

// The client half of the bag.
//
// It owns the item catalog, because that is session-lifetime data every part
// of the UI needs to look names and bonuses up in, and it would be wrong for
// a scene (which comes and goes) to hold it. The inventory CONTENTS are not
// held here - they are handed to whoever registered the update callback, so
// there is exactly one owner of that state rather than a copy here and a copy
// in the UI that can disagree.
class InventoryController
{
public:
    using InventoryUpdateCallback = std::function<void(const std::vector<OwnedItem>&, const CharacterStats&)>;
    using ActionResultCallback = std::function<void(bool pIsSuccess)>;

    InventoryController(NetworkManager* pNetworkManager);

    void initialize();

    void sendEquipItem(uint32_t pBagSlot);
    void sendUnequipItem(EquipSlotType pEquipSlot, std::optional<uint32_t> pBagSlot = std::nullopt);
    void sendMoveItem(uint32_t pFromSlot, uint32_t pToSlot);

    void setInventoryUpdateCallback(InventoryUpdateCallback pCallback);
    void setActionResultCallback(ActionResultCallback pCallback);

    // See AuthController::clearCallbacks - same dangling-`this` hazard, same
    // reason. Called from WorldScene::exit().
    void clearCallbacks();

    // nullptr for an id the catalog does not know. The catalog arrives before
    // the first inventory update (see the server's AuthController), so a miss
    // means genuinely bad data rather than a race.
    const ItemTemplate* findTemplate(uint32_t pTemplateId) const;

    bool hasTemplates() const;

    // The bag's shape as the server declared it, alongside the catalog.
    const InventoryGrid& getGrid() const;

private:
    NetworkManager* mNetworkManager;

    // Sent once per session and immutable afterwards, like the server's own
    // ItemCatalog.
    std::unordered_map<uint32_t, ItemTemplate> mTemplates;
    InventoryGrid mGrid;

    InventoryUpdateCallback mInventoryUpdateCallback;
    ActionResultCallback mActionResultCallback;

    void handleItemTemplateList(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleInventoryUpdate(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleActionResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
};

}

#endif
