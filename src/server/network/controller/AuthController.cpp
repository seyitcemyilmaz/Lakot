#include "AuthController.h"

using namespace lakot;

AuthController::AuthController(NetworkManager& pNetworkManager)
    : BaseController(pNetworkManager)
{

}

void AuthController::initialize()
{
    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kLoginRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleLoginRequest(pSession, pMessage);
        }
    );
}

void AuthController::handleLoginRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    const auto& tLoginReq = pMessage.request().login_request();
    std::string tUsername = tLoginReq.username();

    uint64_t tUserId = std::hash<std::string>{}(tUsername);

    std::cout << "[Server] Giris Istegi: " << tUsername << " (ID: " << tUserId << ")" << std::endl;

    pSession->setUserId(tUserId);

    auto tOldSession = mNetworkManager.getSessionRegistry().addOrReplace(tUserId, pSession);

    if (tOldSession)
    {
        if (tOldSession != pSession)
        {
            std::cout << "[Server] Cakisman oturum. Eski baglanti kapatiliyor." << std::endl;
            tOldSession->close();
        }
    }

    connection::Message tResponse;
    auto* tHeader = tResponse.mutable_response()->mutable_header();

    tHeader->set_request_id(pMessage.request().header().request_id());
    tHeader->mutable_status()->set_code(common::STATUS_OK);

    tResponse.mutable_response()->mutable_login_response()->set_token("TOKEN_LAKOT_123");

    pSession->send(tResponse);
}
