#include "AuthController.h"

using namespace lakot;

AuthController::AuthController(MessageDispatcher<connection::Message>* pMessageDispatcher)
    : mMessageDispatcher(pMessageDispatcher)
{

}

void AuthController::initialize()
{
    // mMessageDispatcher->registerHandler(protocol::Request::kLoginRequest, PacketType::Request,
    //     [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
    //     {
    //         this->handleLoginRequest(pSession, pMessage);
    //     }
    // );

    mMessageDispatcher->registerHandler(protocol::Response::kLoginResponse, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleLoginResponse(pSession, pMessage);
        }
    );
}

// void AuthController::handleLoginRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
// {
//     const auto& loginRequest = pMessage.request().login_request();
//     std::cout << "Login istegi: " << loginRequest.username() << std::endl;
// }

void AuthController::handleLoginResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message &pMessage)
{
    const auto& loginResponse = pMessage.response().login_response();
    std::cout << "Login cevabi: " << loginResponse.token() << std::endl;
}
