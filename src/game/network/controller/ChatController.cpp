#include "ChatController.h"

#include <SDL3/SDL.h>

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

    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kChatMessageReceived, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleChatMessageReceived(pSession, pMessage);
        }
    );

    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kChatMessageResponse, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleChatMessageResponse(pSession, pMessage);
        }
    );
}

uint64_t ChatController::sendDirectMessage(const std::string& pToUsername, const std::string& pText)
{
    auto tSession = mNetworkManager->getClient().getSession();

    if (!tSession)
    {
        // Deliberately does NOT fire mDirectMessageResultCallback: that
        // callback is now matched by request id, and there is no id for a
        // message that never went out, so every window would ignore it. The 0
        // return is the caller's signal to report the failure itself, in the
        // window the player actually typed into.
        return 0;
    }

    uint64_t tRequestId = mNetworkManager->nextRequestId();

    connection::Message tMessage;
    auto* tRequest = tMessage.mutable_request();
    // The server echoes this back as ResponseHeader.reply_to (it already did;
    // the client just never filled the id in, which is why every whisper
    // response was uncorrelated).
    tRequest->mutable_header()->set_id(tRequestId);
    tRequest->mutable_direct_message_request()->set_message_to(pToUsername);
    tRequest->mutable_direct_message_request()->set_text(pText);

    tSession->send(tMessage);

    return tRequestId;
}

void ChatController::sendChatMessage(const std::string& pText, bool pIsGlobal)
{
    auto tSession = mNetworkManager->getClient().getSession();

    if (!tSession)
    {
        return;
    }

    connection::Message tMessage;
    auto* tRequest = tMessage.mutable_request()->mutable_chat_message_request();
    tRequest->set_text(pText);
    tRequest->set_is_global(pIsGlobal);

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

void ChatController::setChatMessageReceivedCallback(ChatMessageReceivedCallback pCallback)
{
    mChatMessageReceivedCallback = pCallback;
}

void ChatController::setChatMessageResultCallback(ChatMessageResultCallback pCallback)
{
    mChatMessageResultCallback = pCallback;
}

void ChatController::clearCallbacks()
{
    mDirectMessageReceivedCallback = nullptr;
    mDirectMessageResultCallback = nullptr;
    mChatMessageReceivedCallback = nullptr;
    mChatMessageResultCallback = nullptr;
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
    uint64_t tRequestId = pMessage.response().header().reply_to();
    bool tIsSuccess = (tResponse.error_code() == services::chat::DIRECT_MESSAGE_OK);

    // DIRECT_MESSAGE_DISCONNECTED covers both "no such username" and "found
    // but not currently online" (see the server's own handleDirectMessageRequest -
    // both collapse to this one code). UI copy stays English, per standing
    // preference - not Turkish, even where a request happened to be phrased
    // in Turkish.
    std::string tMessage = tIsSuccess ? "" :
        (tResponse.error_code() == services::chat::DIRECT_MESSAGE_DISCONNECTED ? "User is not active." : "Message could not be sent.");

    mDirectMessageResultCallback(tRequestId, tIsSuccess, tMessage);
}

void ChatController::handleChatMessageReceived(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mChatMessageReceivedCallback)
    {
        return;
    }

    const auto& tReceived = pMessage.response().chat_message_received();

    mChatMessageReceivedCallback(tReceived.from_player_id(), tReceived.from_username(), tReceived.text(), tReceived.is_global());
}

void ChatController::handleChatMessageResponse(std::shared_ptr<NetworkSession<connection::Message>> /*pSession*/, const connection::Message& pMessage)
{
    const auto& tResponse = pMessage.response().chat_message_response();

    bool tIsSuccess = (tResponse.error_code() == services::chat::CHAT_MESSAGE_OK);

    if (tIsSuccess)
    {
        if (mChatMessageResultCallback)
        {
            mChatMessageResultCallback(true, "");
        }

        return;
    }

    // The server explains WHY in the header status (how many seconds are left
    // on the world-message cooldown, for instance). Falling back to a generic
    // line only if it sent none.
    std::string tMessage = pMessage.response().header().status().message();

    if (tMessage.empty())
    {
        tMessage = "Message could not be sent.";
    }

    if (mChatMessageResultCallback)
    {
        mChatMessageResultCallback(false, tMessage);
    }
    else
    {
        SDL_Log("ChatController: world chat message failed to send (%s)", tMessage.c_str());
    }
}
