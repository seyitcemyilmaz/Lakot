#include "ChatController.h"

#include "WorldController.h"

using namespace lakot;

ChatController::ChatController(NetworkManager& pNetworkManager, RepositoryManager& pRepositoryManager)
    : BaseController(pNetworkManager, pRepositoryManager)
{

}

void ChatController::initialize()
{
    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kDirectMessageRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleDirectMessageRequest(pSession, pMessage);
        }
    );
}

void ChatController::setWorldController(WorldController& pWorldController)
{
    mWorldController = &pWorldController;
}

void ChatController::handleDirectMessageRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mWorldController)
    {
        return;
    }

    uint64_t tSenderId = pSession->getUserId();

    if (tSenderId == 0)
    {
        return; // not logged in yet - ignore
    }

    const auto& tRequest = pMessage.request().direct_message_request();

    connection::Message tAckMessage;
    auto* tAckHeader = tAckMessage.mutable_response()->mutable_header();
    tAckHeader->set_reply_to(pMessage.request().header().id());
    tAckHeader->mutable_status()->set_code(common::STATUS_OK);
    auto* tAck = tAckMessage.mutable_response()->mutable_direct_message_response();

    auto tTargetId = mWorldController->findOnlinePlayerIdByUsername(tRequest.message_to());
    auto tTargetSession = tTargetId ? mNetworkManager.getSessionRegistry().get(*tTargetId) : nullptr;

    if (!tTargetSession)
    {
        tAck->set_error_code(services::chat::DIRECT_MESSAGE_DISCONNECTED);
        pSession->send(tAckMessage);
        return;
    }

    connection::Message tPushMessage;
    auto* tReceived = tPushMessage.mutable_response()->mutable_direct_message_received();
    tReceived->set_from_player_id(tSenderId);
    tReceived->set_from_username(mWorldController->getPlayerUsername(tSenderId));
    tReceived->set_text(tRequest.text());

    tTargetSession->send(tPushMessage);

    tAck->set_error_code(services::chat::DIRECT_MESSAGE_OK);
    pSession->send(tAckMessage);
}
