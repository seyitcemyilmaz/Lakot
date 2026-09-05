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
    using DirectMessageResultCallback = std::function<void(bool pIsSuccess, const std::string& pMessage)>;

    ChatController(NetworkManager* pNetworkManager);

    void initialize();

    void sendDirectMessage(const std::string& pToUsername, const std::string& pText);

    void setDirectMessageReceivedCallback(DirectMessageReceivedCallback pCallback);
    void setDirectMessageResultCallback(DirectMessageResultCallback pCallback);

private:
    NetworkManager* mNetworkManager;

    DirectMessageReceivedCallback mDirectMessageReceivedCallback;
    DirectMessageResultCallback mDirectMessageResultCallback;

    void handleDirectMessageReceived(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleDirectMessageResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
};

}

#endif
