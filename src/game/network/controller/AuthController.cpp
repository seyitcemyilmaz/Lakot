#include "AuthController.h"

#include "../NetworkManager.h"

using namespace lakot;

AuthController::AuthController(NetworkManager* pNetworkManager)
    : mNetworkManager(pNetworkManager)
{

}

void AuthController::initialize()
{
    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kLoginResponse, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleLoginResponse(pSession, pMessage);
        }
    );

    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kRegisterResponse, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleRegisterResponse(pSession, pMessage);
        }
    );
}

void AuthController::sendRegisterAccount(const std::string& pUsername, const std::string& pPassword, const std::string& pEmail)
{
    auto tSession = mNetworkManager->getClient().getSession();

    if (!tSession)
    {
        if (mRegisterResultCallback)
        {
            mRegisterResultCallback(false, "No connection to server!");
        }
        return;
    }

    connection::Message tMessage;
    auto* tRequest = tMessage.mutable_request();
    tRequest->mutable_register_request()->set_username(pUsername);
    tRequest->mutable_register_request()->set_password(pPassword);
    tRequest->mutable_register_request()->set_email(pEmail);

    tSession->send(tMessage);
}

void AuthController::setRegisterResultCallback(RegisterResultCallback pCallback)
{
    mRegisterResultCallback = pCallback;
}

void AuthController::sendLoginRequest(const std::string& pUsername, const std::string& pPassword)
{
    auto tSession = mNetworkManager->getClient().getSession();

    if (!tSession)
    {
        if (mLoginResultCallback)
        {
            mLoginResultCallback(false, "No connection to server!", LoginSpawnState{});
        }
        return;
    }

    connection::Message tMessage;
    auto* tRequest = tMessage.mutable_request();
    tRequest->mutable_login_request()->set_username(pUsername);
    tRequest->mutable_login_request()->set_password(pPassword);

    tSession->send(tMessage);
}

void AuthController::setLoginResultCallback(LoginResultCallback pCallback)
{
    mLoginResultCallback = pCallback;
}

void AuthController::handleLoginResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    const auto& tLoginResponse = pMessage.response().login_response();

    bool tIsSuccess = !tLoginResponse.token().empty();
    std::string tMessage = tIsSuccess ? "Login Successful!" : "Invalid credentials!";

    LoginSpawnState tSpawnState{
        tLoginResponse.map_id(),
        tLoginResponse.pos_x(),
        tLoginResponse.pos_y(),
        tLoginResponse.pos_z(),
        tLoginResponse.yaw()
    };

    if (mLoginResultCallback)
    {
        mLoginResultCallback(tIsSuccess, tMessage, tSpawnState);
    }
}

void AuthController::handleRegisterResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    const auto& tResponse = pMessage.response().register_response();

    if (mRegisterResultCallback)
    {
        if (tResponse.register_status() == services::auth::REGISTER_STATUS_OK)
        {
            mRegisterResultCallback(true, "Registration successful.");
        }
        else
        {
            mRegisterResultCallback(false, "Registration failed.");
        }
    }
}
