#ifndef LAKOT_AUTHCONTROLLER_H
#define LAKOT_AUTHCONTROLLER_H

#include <connection.pb.h>

namespace lakot
{

class NetworkManager;

template <typename MessageType>
class NetworkSession;

class AuthController
{
public:
    using LoginResultCallback = std::function<void(bool, const std::string&)>;

    AuthController(NetworkManager* pNetworkManager);

    void initialize();

    void login(const std::string& pUsername, const std::string& pPassword);

    void setLoginResultCallback(LoginResultCallback pCallback);

private:
    NetworkManager* mNetworkManager;

    LoginResultCallback mLoginResultCallback;

    void handleLoginResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);

};

}

#endif
