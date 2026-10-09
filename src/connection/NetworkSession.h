#ifndef LAKOT_NETWORKSESSION_H
#define LAKOT_NETWORKSESSION_H

#include <iostream>
#include <deque>
#include <atomic>
#include <memory>
#include <vector>
#include <syncstream>

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

    // An already-framed, already-serialized packet. Broadcasting the same
    // message to N recipients used to serialize it N times (ByteSizeLong +
    // SerializeToArray inside every send()); with makePacket() the work is
    // done once and every recipient shares the buffer. At MMO scale this is
    // the difference between one serialization per world tick and one per
    // (player x nearby player) pair.
    using Packet = std::shared_ptr<const std::vector<char>>;

    NetworkSession(boost::asio::ip::tcp::socket pSocket)
        : mSocket(std::move(pSocket))
        , mStrand(boost::asio::make_strand(mSocket.get_executor()))
        , mIsConnected(true)
    {
        boost::system::error_code tErrorCode;
        mSocket.set_option(boost::asio::ip::tcp::no_delay(true), tErrorCode);
        mSocket.set_option(boost::asio::socket_base::keep_alive(true), tErrorCode);

        if (tErrorCode)
        {
            std::osyncstream(std::cout) << tErrorCode << std::endl;
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

    // Frames pMessage (4-byte big-endian length prefix + body) once, so the
    // result can be handed to any number of sessions. Returns nullptr if
    // serialization failed.
    static Packet makePacket(const MessageType& pMessage)
    {
        size_t tBodySize = pMessage.ByteSizeLong();
        uint32_t tHeader = htonl(static_cast<uint32_t>(tBodySize));

        auto tPacket = std::make_shared<std::vector<char>>(sizeof(tHeader) + tBodySize);

        std::memcpy(tPacket->data(), &tHeader, sizeof(tHeader));

        if (!pMessage.SerializeToArray(tPacket->data() + sizeof(tHeader), static_cast<int>(tBodySize)))
        {
            return nullptr;
        }

        return tPacket;
    }

    void send(const MessageType& pMessage)
    {
        if (!mIsConnected.load())
        {
            return;
        }

        send(makePacket(pMessage));
    }

    void send(Packet pPacket)
    {
        if (!pPacket || !mIsConnected.load())
        {
            return;
        }

        mPendingWrites.fetch_add(1);

        boost::asio::post(mStrand,
        [self = this->shared_from_this(), packet = std::move(pPacket)]() mutable
        {
            if (!self->mIsConnected.load())
            {
                self->mPendingWrites.fetch_sub(1);
                return;
            }

            // Backpressure. The write queue used to be unbounded: a client
            // that stops reading (slow link, or deliberately) makes every
            // subsequent broadcast pile up here forever, and with thousands
            // of sessions that is an out-of-memory kill, not a slowdown.
            // A healthy client never comes close to this; one that does is
            // dropped rather than allowed to consume the server.
            if (self->mWriteQueueBytes + packet->size() > kMaxWriteQueueBytes)
            {
                std::osyncstream(std::cerr) << "[Session] Yazma kuyrugu doldu, baglanti kapatiliyor. Account: "
                          << self->mAccountId << std::endl;

                self->mPendingWrites.fetch_sub(1);
                self->onDisconnect(boost::asio::error::no_buffer_space);
                return;
            }

            bool tWriteInProgress = !self->mWriteQueue.empty();

            self->mWriteQueueBytes += packet->size();
            self->mWriteQueue.push_back(std::move(packet));

            if (!tWriteInProgress)
            {
                self->doWrite();
            }
        });
    }

    bool hasPendingWrites() const
    {
        return mIsConnected.load() && mPendingWrites.load() > 0;
    }

    void setOnMessageFunction(MessageHandler pHandler)
    {
        mOnMessageFunction = pHandler;
    }

    void setOnErrorFunction(ErrorHandler pHandler)
    {
        mOnErrorFunction = pHandler;
    }

    bool isConnected() const
    {
        return mIsConnected.load();
    }

    boost::asio::ip::tcp::socket& getSocket()
    {
        return mSocket;
    }

    // Two identities, set at two different points of the session's life, and
    // both needed at once: the account is who authenticated (one live session
    // per account, which is what the relogin kick is enforced on), the
    // character is which of that account's characters is currently in the
    // world (what other players see, whisper to, and share a zone with).
    // 0 means "not yet" - authenticated-but-at-character-select is a real and
    // normal state, and world traffic must not be accepted in it.
    void setAccountId(uint64_t pAccountId)
    {
        mAccountId = pAccountId;
    }

    uint64_t getAccountId() const
    {
        return mAccountId;
    }

    void setCharacterId(uint64_t pCharacterId)
    {
        mCharacterId = pCharacterId;
    }

    uint64_t getCharacterId() const
    {
        return mCharacterId;
    }

private:
    // Declaration order is the initialization order, regardless of what the
    // constructor's initializer list says - mStrand is built from
    // mSocket.get_executor(), so mSocket must stay above it, and the list
    // above is written to match this order exactly.
    boost::asio::ip::tcp::socket mSocket;
    boost::asio::strand<boost::asio::ip::tcp::socket::executor_type> mStrand;

    std::atomic<bool> mIsConnected;

    // Per-session ceiling on unsent, already-queued bytes - see send(Packet).
    // Generous for normal play (a world snapshot is a few hundred bytes), so
    // only a client that has genuinely stopped draining its socket hits it.
    static constexpr size_t kMaxWriteQueueBytes = 1 * 1024 * 1024;

    uint32_t mIncomingHeader{0};
    std::vector<char> mIncomingBody;

    std::deque<Packet> mWriteQueue;
    size_t mWriteQueueBytes{0};

    std::atomic<size_t> mPendingWrites{0};

    MessageHandler mOnMessageFunction;
    ErrorHandler mOnErrorFunction;

    uint64_t mAccountId{0};
    uint64_t mCharacterId{0};

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
                                 boost::asio::buffer(*mWriteQueue.front()),
                                 boost::asio::bind_executor(mStrand,
        [self](boost::system::error_code pErrorCode, std::size_t)
        {
            if (!pErrorCode)
            {
                self->mWriteQueueBytes -= self->mWriteQueue.front()->size();
                self->mWriteQueue.pop_front();
                self->mPendingWrites.fetch_sub(1);

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

        mPendingWrites.store(0);

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
};

}

#endif
