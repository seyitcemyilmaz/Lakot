#ifndef LAKOT_SERVER_AUTHCONTROLLER_H
#define LAKOT_SERVER_AUTHCONTROLLER_H

#include <string>

#include <connection.pb.h>

#include "BaseController.h"

#include "../../security/LoginAttemptLimiter.h"
#include "../../security/TokenBucketLimiter.h"

namespace lakot
{

template <typename MessageType>
class NetworkSession;

class WorldController;

class AuthController : public BaseController
{
public:
    AuthController(NetworkManager& pNetworkManager, RepositoryManager& pRepositoryManager, WorldController& pWorldController);

    void initialize() override;

private:
    WorldController& mWorldController;

    static constexpr double kDefaultRegistersPerMinute = 30.0;

    LoginAttemptLimiter mAttemptLimiter;
    TokenBucketLimiter mRegisterLimiter;

    void sendRejection(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                       const connection::Message& pRequestMessage,
                       common::StatusCode pStatusCode,
                       const std::string& pMessage,
                       bool pIsLogin);

    void handleRegisterRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleLoginRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);

    void handleResumeSessionRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleLogoutRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
};

}

#endif
