#include "ChatController.h"

#include "../NetworkManager.h"

using namespace lakot;

ChatController::ChatController(NetworkManager* pNetworkManager)
    : mNetworkManager(pNetworkManager)
{

}

void ChatController::initialize()
{
    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kDirectMessageReceived, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleDirectMessageReceived(pSession, pMessage);
        }
    );

    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kDirectMessageResponse, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleDirectMessageResponse(pSession, pMessage);
        }
    );
}

void ChatController::sendDirectMessage(const std::string& pToUsername, const std::string& pText)
{
    auto tSession = mNetworkManager->getClient().getSession();

    if (!tSession)
    {
        if (mDirectMessageResultCallback)
        {
            mDirectMessageResultCallback(false, "No connection to server!");
        }
        return;
    }

    connection::Message tMessage;
    auto* tRequest = tMessage.mutable_request();
    tRequest->mutable_direct_message_request()->set_message_to(pToUsername);
    tRequest->mutable_direct_message_request()->set_text(pText);

    tSession->send(tMessage);
}

void ChatController::setDirectMessageReceivedCallback(DirectMessageReceivedCallback pCallback)
{
    mDirectMessageReceivedCallback = pCallback;
}

void ChatController::setDirectMessageResultCallback(DirectMessageResultCallback pCallback)
{
    mDirectMessageResultCallback = pCallback;
}

void ChatController::handleDirectMessageReceived(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mDirectMessageReceivedCallback)
    {
        return;
    }

    const auto& tReceived = pMessage.response().direct_message_received();

    mDirectMessageReceivedCallback(tReceived.from_player_id(), tReceived.from_username(), tReceived.text());
}

void ChatController::handleDirectMessageResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mDirectMessageResultCallback)
    {
        return;
    }

    const auto& tResponse = pMessage.response().direct_message_response();
    bool tIsSuccess = (tResponse.error_code() == services::chat::DIRECT_MESSAGE_OK);

    std::string tMessage = tIsSuccess ? "" :
        (tResponse.error_code() == services::chat::DIRECT_MESSAGE_DISCONNECTED ? "User is offline." : "Message could not be sent.");

    mDirectMessageResultCallback(tIsSuccess, tMessage);
}
