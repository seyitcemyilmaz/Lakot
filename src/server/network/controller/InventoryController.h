#ifndef LAKOT_SERVER_INVENTORYCONTROLLER_H
#define LAKOT_SERVER_INVENTORYCONTROLLER_H

#include <memory>

#include <connection.pb.h>

#include "BaseController.h"

namespace lakot
{

template <typename MessageType>
class NetworkSession;

class WorldController;
class ItemCatalog;

// Equip, unequip, and moving an item between bag slots, plus handing the
// client the item catalog once per session.
//
// It owns no item state of its own: a character's items live in the Character
// the zone owns, so every mutation here goes through
// WorldController::withCharacter and therefore executes on that zone's
// thread. This controller only validates the request and reports the result -
// the InventoryUpdate that follows is pushed by the zone itself.
class InventoryController : public BaseController
{
public:
    InventoryController(NetworkManager& pNetworkManager,
                        RepositoryManager& pRepositoryManager,
                        WorldController& pWorldController,
                        const ItemCatalog& pItemCatalog);

    void initialize() override;


    // Sent once, when a character enters the world - the client needs names
    // and bonuses before an InventoryUpdate's template ids mean anything.
    void sendItemTemplates(const std::shared_ptr<NetworkSession<connection::Message>>& pSession);

private:
    WorldController& mWorldController;
    const ItemCatalog& mItemCatalog;

    void handleEquipItemRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleUnequipItemRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleMoveItemRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);

    void sendActionResult(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                          uint64_t pReplyTo,
                          bool pIsSuccess);
};

}

#endif
