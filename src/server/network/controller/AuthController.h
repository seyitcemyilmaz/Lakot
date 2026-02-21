#ifndef LAKOT_SERVER_AUTHCONTROLLER_H
#define LAKOT_SERVER_AUTHCONTROLLER_H

#include <connection.pb.h>

#include "BaseController.h"

namespace lakot
{

template <typename MessageType>
class NetworkSession;

class AuthController : public BaseController
{
public:
    AuthController(NetworkManager& pNetworkManager);

    void initialize() override;

private:
    void handleLoginRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
};

}

#endif
