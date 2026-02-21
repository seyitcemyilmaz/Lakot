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
}

void AuthController::login(const std::string& pUsername, const std::string& pPassword)
{
    auto tSession = mNetworkManager->getClient().getSession();

    if (!tSession)
    {
        if (mLoginResultCallback)
        {
            mLoginResultCallback(false, "No connection to server!");
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
    const auto& loginResponse = pMessage.response().login_response();

    bool tIsSuccess = !loginResponse.token().empty();
    std::string tMessage = tIsSuccess ? "Login Successful!" : "Invalid credentials!";

    if (mLoginResultCallback)
    {
        mLoginResultCallback(tIsSuccess, tMessage);
    }
}
