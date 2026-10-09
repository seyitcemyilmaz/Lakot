#ifndef LAKOT_CHATCONTROLLER_H
#define LAKOT_CHATCONTROLLER_H

#include <cstdint>
#include <functional>
#include <string>

#include <connection.pb.h>

namespace lakot
{

class NetworkManager;

template <typename MessageType>
class NetworkSession;

class ChatController
{
public:
    using DirectMessageReceivedCallback = std::function<void(uint64_t pFromPlayerId, const std::string& pFromUsername, const std::string& pText)>;

    // pRequestId is the id sendDirectMessage() returned for the send this is
    // the answer to, taken from the response's ResponseHeader.reply_to. There
    // is still one callback slot (WorldScene fans it out to every open whisper
    // window), but a window can now tell whether a given result is actually
    // its own instead of every window consuming the first result that arrives.
    using DirectMessageResultCallback = std::function<void(uint64_t pRequestId, bool pIsSuccess, const std::string& pMessage)>;

    // Same shape as the whisper pair above, plus pIsGlobal - "chat" here
    // means the world/say channel: a server-wide message (pIsGlobal=true,
    // the sender either had the scope button set to "World" or typed a
    // leading "!" for that one message - see WorldChatRmlController::doSend())
    // or a plain nearby-only one (pIsGlobal=false).
    using ChatMessageReceivedCallback = std::function<void(uint64_t pFromPlayerId, const std::string& pFromUsername, const std::string& pText, bool pIsGlobal)>;

    // The world-chat counterpart of DirectMessageResultCallback. A rejected
    // world message used to reach nothing but SDL_Log, so the player saw
    // their own optimistically-appended line sitting in the history and had
    // no way to know it never went out - which is exactly how the global
    // cooldown first showed up as a mystery.
    using ChatMessageResultCallback = std::function<void(bool pIsSuccess, const std::string& pMessage)>;

    ChatController(NetworkManager* pNetworkManager);

    void initialize();

    // Returns the request id stamped into the message's RequestHeader, which
    // comes back as ResponseHeader.reply_to on the matching
    // DirectMessageResultCallback - callers hold on to it to recognise their
    // own result. Returns 0 if there was no connection to send on; no result
    // callback fires in that case (there is no id to match it against), so
    // the caller is responsible for surfacing that failure itself.
    uint64_t sendDirectMessage(const std::string& pToUsername, const std::string& pText);
    void sendChatMessage(const std::string& pText, bool pIsGlobal);

    void setDirectMessageReceivedCallback(DirectMessageReceivedCallback pCallback);
    void setDirectMessageResultCallback(DirectMessageResultCallback pCallback);
    void setChatMessageReceivedCallback(ChatMessageReceivedCallback pCallback);
    void setChatMessageResultCallback(ChatMessageResultCallback pCallback);

    // Drops every callback registered above. Callbacks capture the scene/UI
    // object that registered them (WorldScene), but this controller lives as
    // long as the process (it is owned by Engine's NetworkManager) - so a
    // scene that goes away without clearing them leaves this holding dangling
    // `this` pointers, and the next matching packet is a use-after-free.
    // Called from WorldScene::exit().
    void clearCallbacks();

private:
    NetworkManager* mNetworkManager;

    DirectMessageReceivedCallback mDirectMessageReceivedCallback;
    DirectMessageResultCallback mDirectMessageResultCallback;
    ChatMessageReceivedCallback mChatMessageReceivedCallback;
    ChatMessageResultCallback mChatMessageResultCallback;

    void handleDirectMessageReceived(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleDirectMessageResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleChatMessageReceived(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);

    // Was entirely unregistered before - the server acks every sent world-
    // chat message with this (see sendChatMessage()/server ChatController),
    // and with no handler for it, the dispatcher logged
    // "Unhandled Packet Key ... (ID: 2000) from User: 0" every single send.
    // World chat has no dedicated failure UI yet (unlike whisper's
    // DirectMessageResultCallback), so this just logs a real failure instead
    // of silently dropping it - same minimal treatment, not inventing new UI.
    void handleChatMessageResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
};

}

#endif
