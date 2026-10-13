#ifndef LAKOT_SERVER_CHARACTERCONTROLLER_H
#define LAKOT_SERVER_CHARACTERCONTROLLER_H

#include <connection.pb.h>

#include "BaseController.h"

#include "../../game/Character.h"

namespace lakot
{

class ItemCatalog;
class ClassCatalog;
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
                        const ClassCatalog& pClassCatalog,
                        const MapCatalog& pMapCatalog);

    void initialize() override;

private:
    WorldController& mWorldController;
    InventoryController& mInventoryController;
    const ItemCatalog& mItemCatalog;
    const ClassCatalog& mClassCatalog;
    const MapCatalog& mMapCatalog;

    void handleCharacterListRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleCharacterCreateRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleEnterWorldRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleDeleteCharacterRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);

    static void sendDeleteResult(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                                 uint64_t pReplyTo,
                                 services::auth::DeleteCharacterStatus pStatus,
                                 uint64_t pCharacterId);

    static void sendEnterWorldFailure(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                                      uint64_t pReplyTo,
                                      common::StatusCode pStatusCode,
                                      services::auth::EnterWorldStatus pEnterStatus);

    void sendCharacterList(const std::shared_ptr<NetworkSession<connection::Message>>& pSession, uint64_t pReplyTo);
};

}

#endif
