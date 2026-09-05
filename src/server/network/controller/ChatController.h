#ifndef LAKOT_SERVER_CHATCONTROLLER_H
#define LAKOT_SERVER_CHATCONTROLLER_H

#include <connection.pb.h>

#include "BaseController.h"

namespace lakot
{

template <typename MessageType>
class NetworkSession;

class WorldController;

class ChatController : public BaseController
{
public:
    ChatController(NetworkManager& pNetworkManager, RepositoryManager& pRepositoryManager);

    void initialize() override;

    // Same wiring pattern as AuthController::setWorldController - lets this
    // controller resolve a whisper's target username to an online player id
    // (and the sender's own username) without duplicating that bookkeeping.
    void setWorldController(WorldController& pWorldController);

private:
    WorldController* mWorldController{nullptr};

    void handleDirectMessageRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
};

}

#endif
