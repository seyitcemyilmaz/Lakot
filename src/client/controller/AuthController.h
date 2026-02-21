#ifndef LAKOT_CLIENT_AUTHCONTROLLER_H
#define LAKOT_CLIENT_AUTHCONTROLLER_H

#include <connection.pb.h>

#include "NetworkSession.h"
#include "MessageDispatcher.h"

class Session;

namespace lakot
{

class AuthController
{
public:
    AuthController(MessageDispatcher<connection::Message>* pMessageDispatcher);

    void initialize();

private:
    MessageDispatcher<connection::Message>* mMessageDispatcher;

    // void handleLoginRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleLoginResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);

};

}

#endif
