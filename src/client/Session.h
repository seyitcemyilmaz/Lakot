#ifndef SESSION_H
#define SESSION_H

#include <memory>
#include <deque>
#include <vector>
#include <functional>

#include <boost/asio.hpp>

#include "connection.pb.h"

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(boost::asio::ip::tcp::socket pSocket);
    virtual ~Session();

    void start();
    void close();
    void send(const connection::Message& pMessage);

    using ResponseCallback = std::function<void(const connection::Message&)>;
    void sendRequest(connection::Message& pMessage, ResponseCallback pCallback);

    bool isConnected() const;

    using MessageHandler = std::function<void(std::shared_ptr<Session>, const connection::Message&)>;
    void setMessageHandler(MessageHandler pHandler);

    using DisconnectHandler = std::function<void()>;
    void setDisconnectHandler(DisconnectHandler pHandler);

private:
    static constexpr uint32_t MAX_PACKET_SIZE = 10 * 1024 * 1024; // 10 MB Limit

    boost::asio::ip::tcp::socket mSocket;
    boost::asio::strand<boost::asio::ip::tcp::socket::executor_type> mStrand;

    uint32_t mIncomingHeader;
    std::vector<char> mIncomingBody;

    std::deque<std::string> mWriteQueue;

    MessageHandler mMessageHandler;

    DisconnectHandler mDisconnectHandler;

    std::atomic<uint64_t> mNextRequestId{1};

    std::map<uint64_t, ResponseCallback> mPendingRequests;
    std::mutex mRequestMutex;

    void handleIncomingMessage(connection::Message& pMessage);

    void doReadHeader();
    void doReadBody();

    void doWrite();
};

#endif
