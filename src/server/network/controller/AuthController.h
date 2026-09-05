#ifndef LAKOT_SERVER_AUTHCONTROLLER_H
#define LAKOT_SERVER_AUTHCONTROLLER_H

#include <connection.pb.h>

#include "BaseController.h"

namespace lakot
{

template <typename MessageType>
class NetworkSession;

class WorldController;

class AuthController : public BaseController
{
public:
    AuthController(NetworkManager& pNetworkManager, RepositoryManager& pRepositoryManager);

    void initialize() override;

    // Set once from Server's constructor, after both controllers exist -
    // lets a successful login seed WorldController's map assignment for the
    // player (from their restored state) before the login response goes
    // out, so their first PlayerStateUpdate lands in the right map.
    void setWorldController(WorldController& pWorldController);

private:
    WorldController* mWorldController = nullptr;

    void handleRegisterRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);

    void handleLoginRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
};

}

#endif
