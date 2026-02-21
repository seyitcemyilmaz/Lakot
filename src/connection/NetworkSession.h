#ifndef LAKOT_NETWORKSESSION_H
#define LAKOT_NETWORKSESSION_H

#include <iostream>
#include <map>
#include <deque>
#include <atomic>

#include <boost/asio.hpp>

#include "connection_global.h"

namespace lakot
{

template <typename MessageType>
class LAKOT_CONNECTION_EXPORT NetworkSession : public std::enable_shared_from_this<NetworkSession<MessageType>>
{
public:
    using Ptr = std::shared_ptr<NetworkSession<MessageType>>;
    using MessageHandler = std::function<void(Ptr, const MessageType&)>;
    using ErrorHandler = std::function<void(Ptr, const boost::system::error_code&)>;
    using ResponseCallback = std::function<void(const MessageType&)>;
    using RequestIdSetter = std::function<void(MessageType&, uint32_t)>;

    NetworkSession(boost::asio::ip::tcp::socket pSocket)
        : mSocket(std::move(pSocket))
        , mStrand(boost::asio::make_strand(mSocket.get_executor()))
        , mIsConnected(true)
        , mNextRequestId(1)
    {
        boost::system::error_code tErrorCode;
        mSocket.set_option(boost::asio::ip::tcp::no_delay(true), tErrorCode);
        mSocket.set_option(boost::asio::socket_base::keep_alive(true), tErrorCode);

        if (tErrorCode)
        {
            std::cout << tErrorCode << std::endl;
        }
    }

    void start()
    {
        readHeader();
    }

    void close()
    {
        boost::asio::post(mStrand,
                          [self = this->shared_from_this()]()
        {
            if (self->mSocket.is_open())
            {
                boost::system::error_code tErrorCode;
                self->mSocket.close(tErrorCode);
            }
        });
    }

    void send(const MessageType& pMessage)
    {
        if (!mIsConnected.load())
        {
            return;
        }

        int tBodySize = static_cast<int>(pMessage.ByteSizeLong());
        uint32_t tHeader = htonl(static_cast<uint32_t>(tBodySize));

        std::vector<char> tPacket(sizeof(tHeader) + tBodySize);

        std::memcpy(tPacket.data(), &tHeader, sizeof(tHeader));

        if (!pMessage.SerializeToArray(tPacket.data() + sizeof(tHeader), tBodySize))
        {
            return;
        }

        boost::asio::post(mStrand,
        [self = this->shared_from_this(), packet = std::move(tPacket)]() mutable
        {
            if (!self->mIsConnected.load())
            {
                return;
            }

            bool tWriteInProgress = !self->mWriteQueue.empty();

            self->mWriteQueue.push_back(std::move(packet));

            if (!tWriteInProgress)
            {
                self->doWrite();
            }
        });
    }

    void sendRequest(MessageType& pMessage, ResponseCallback pResponseCallback, uint64_t pRequestId = 0)
    {
        uint32_t tFinalId = pRequestId;

        if (tFinalId == 0)
        {
            tFinalId = generateRequestId();

            if (mRequestIdSetter)
            {
                mRequestIdSetter(pMessage, tFinalId);
            }
        }

        {
            std::lock_guard<std::mutex> tLock(mRequestMutex);
            mPendingRequests[pRequestId] = std::move(pResponseCallback);
        }

        send(pMessage);
    }

    void setOnMessageFunction(MessageHandler pHandler)
    {
        mOnMessageFunction = pHandler;
    }

    void setOnErrorFunction(ErrorHandler pHandler)
    {
        mOnErrorFunction = pHandler;
    }

    void setRequestIdSetter(RequestIdSetter pSetter)
    {
        mRequestIdSetter = pSetter;
    }

    bool isConnected() const
    {
        return mIsConnected.load();
    }

    boost::asio::ip::tcp::socket& getSocket()
    {
        return mSocket;
    }

    void setUserId(uint64_t pUserId)
    {
        mUserId = pUserId;
    }

    uint64_t getUserId() const
    {
        return mUserId;
    }

private:
    std::atomic<bool> mIsConnected;

    boost::asio::ip::tcp::socket mSocket;
    boost::asio::strand<boost::asio::ip::tcp::socket::executor_type> mStrand;

    uint32_t mIncomingHeader{0};
    std::vector<char> mIncomingBody;
    std::deque<std::vector<char>> mWriteQueue;

    MessageHandler mOnMessageFunction;
    ErrorHandler mOnErrorFunction;

    std::mutex mRequestMutex;
    std::map<uint64_t, ResponseCallback> mPendingRequests;

    std::atomic<uint32_t> mNextRequestId;
    RequestIdSetter mRequestIdSetter;

    uint64_t mUserId{0};

    void readHeader()
    {
        auto self = this->shared_from_this();

        boost::asio::async_read(mSocket,
                                boost::asio::buffer(&mIncomingHeader, sizeof(mIncomingHeader)),
                                boost::asio::bind_executor(mStrand,
        [self](boost::system::error_code pErrorCode, std::size_t)
        {
            if (!pErrorCode)
            {
                uint32_t tBodySize = ntohl(self->mIncomingHeader);

                if (tBodySize > 10 * 1024 * 1024)
                {
                    self->close();
                    return;
                }

                self->mIncomingBody.resize(tBodySize);
                self->readBody();
            }
            else
            {
                self->onDisconnect(pErrorCode);
            }
        }));
    }

    void readBody()
    {
        auto self = this->shared_from_this();

        boost::asio::async_read(mSocket,
                                boost::asio::buffer(mIncomingBody),
                                boost::asio::bind_executor(mStrand,
        [self](boost::system::error_code pErrorCode, std::size_t)
        {
            if (!pErrorCode)
            {
                MessageType tMessage;

                if (tMessage.ParseFromArray(self->mIncomingBody.data(), (int)self->mIncomingBody.size()))
                {
                    self->handleIncomingMessage(tMessage);
                }

                self->readHeader();
            }
            else
            {
                self->onDisconnect(pErrorCode);
            }
        }));
    }

    void doWrite()
    {
        auto self = this->shared_from_this();

        boost::asio::async_write(mSocket,
                                 boost::asio::buffer(mWriteQueue.front()),
                                 boost::asio::bind_executor(mStrand,
        [self](boost::system::error_code pErrorCode, std::size_t)
        {
            if (!pErrorCode)
            {
                self->mWriteQueue.pop_front();
                if (!self->mWriteQueue.empty())
                {
                    self->doWrite();
                }
            }
            else
            {
                self->onDisconnect(pErrorCode);
            }
        }));
    }

    void handleIncomingMessage(const MessageType& pMessage)
    {
        if (mOnMessageFunction)
        {
            mOnMessageFunction(this->shared_from_this(), pMessage);
        }
    }

    void onDisconnect(const boost::system::error_code& pErrorCode)
    {
        if (!mIsConnected.exchange(false))
        {
            return;
        }

        if (mSocket.is_open())
        {
            boost::system::error_code tIgnoredEc;
            mSocket.close(tIgnoredEc);
        }

        if (mOnErrorFunction)
        {
            mOnErrorFunction(this->shared_from_this(), pErrorCode);
        }
    }

    uint32_t generateRequestId()
    {
        return mNextRequestId++;
    }
};

}

#endif
