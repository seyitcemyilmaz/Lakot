#ifndef LAKOT_AUTHCONTROLLER_H
#define LAKOT_AUTHCONTROLLER_H

#include <cstdint>

#include <connection.pb.h>

namespace lakot
{

class NetworkManager;

template <typename MessageType>
class NetworkSession;

class AuthController
{
public:
    // Where the account was last known to be - restored from the server's
    // LoginResponse, used to spawn the player back where they left off.
    struct LoginSpawnState
    {
        uint32_t mapId;
        float x;
        float y;
        float z;
        float yaw;
    };

    using LoginResultCallback = std::function<void(bool, const std::string&, const LoginSpawnState&)>;
    using RegisterResultCallback = std::function<void(bool, const std::string&)>;

    AuthController(NetworkManager* pNetworkManager);

    void initialize();

    void sendRegisterAccount(const std::string& pUsername, const std::string& pPassword, const std::string& pEmail);
    void setRegisterResultCallback(RegisterResultCallback pCallback);

    void sendLoginRequest(const std::string& pUsername, const std::string& pPassword);
    void setLoginResultCallback(LoginResultCallback pCallback);

private:
    NetworkManager* mNetworkManager;

    LoginResultCallback mLoginResultCallback;
    RegisterResultCallback mRegisterResultCallback;

    void handleLoginResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleRegisterResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);

};

}

#endif
