#ifndef LAKOT_SERVER_CHARACTERCONTROLLER_H
#define LAKOT_SERVER_CHARACTERCONTROLLER_H

#include <connection.pb.h>

#include "BaseController.h"

#include "../../game/Character.h"

namespace lakot
{

class ItemCatalog;
class MapCatalog;
class InventoryController;
class WorldController;

template <typename MessageType>
class NetworkSession;

class CharacterController : public BaseController
{
public:
    CharacterController(NetworkManager& pNetworkManager,
                        RepositoryManager& pRepositoryManager,
                        WorldController& pWorldController,
                        InventoryController& pInventoryController,
                        const ItemCatalog& pItemCatalog,
                        const MapCatalog& pMapCatalog);

    void initialize() override;

private:
    WorldController& mWorldController;
    InventoryController& mInventoryController;
    const ItemCatalog& mItemCatalog;
    const MapCatalog& mMapCatalog;

    static void fillStats(const Character& pCharacter, services::game::CharacterStats* pOut);

    void handleCharacterListRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleCharacterCreateRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleEnterWorldRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);

    void sendCharacterList(const std::shared_ptr<NetworkSession<connection::Message>>& pSession, uint64_t pReplyTo);
};

}

#endif
