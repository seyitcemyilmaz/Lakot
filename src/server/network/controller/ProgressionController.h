#ifndef LAKOT_SERVER_PROGRESSIONCONTROLLER_H
#define LAKOT_SERVER_PROGRESSIONCONTROLLER_H

#include <memory>

#include <connection.pb.h>

#include "BaseController.h"

namespace lakot
{

template <typename MessageType>
class NetworkSession;

class WorldController;

class ProgressionController : public BaseController
{
public:
    ProgressionController(NetworkManager& pNetworkManager,
                          RepositoryManager& pRepositoryManager,
                          WorldController& pWorldController);

    void initialize() override;

private:
    WorldController& mWorldController;

    void handleAllocateStatRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleChoosePathRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);

    static void sendAllocateStatResult(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                                       uint64_t pReplyTo,
                                       services::progression::AllocateStatStatus pStatus);

    static void sendChoosePathResult(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                                     uint64_t pReplyTo,
                                     services::progression::ChoosePathStatus pStatus);
};

}

#endif
