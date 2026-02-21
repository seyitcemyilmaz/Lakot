#include "Session.h"

#include "MainThreadExecutor.h"

Session::Session(boost::asio::ip::tcp::socket pSocket)
    : mSocket(std::move(pSocket))
    , mStrand(boost::asio::make_strand(mSocket.get_executor()))
    , mIncomingHeader(0)
{
}

Session::~Session()
{
    std::cout << "Session destroyed." << std::endl;
}

void Session::start()
{
    doReadHeader();
}

void Session::close()
{
    boost::asio::post(mStrand,
    [self = shared_from_this()]()
    {
        if (self->mSocket.is_open())
        {
            boost::system::error_code pErrorCode;
            self->mSocket.close(pErrorCode);

            if (self->mDisconnectHandler)
            {
                self->mDisconnectHandler();
                self->mDisconnectHandler = nullptr;
            }
        }
    });
}

// void Session::send(const connection::Message& pMessage)
// {
//     std::string serializedData;
//     if (!pMessage.SerializeToString(&serializedData))
//     {
//         std::cerr << "Protobuf serialization failed!" << std::endl;
//         return;
//     }

//     // Network Byte Order - Big Endian
//     uint32_t tBodySize = static_cast<uint32_t>(serializedData.size());
//     uint32_t tHeader = htonl(tBodySize);

//     std::string tPacket;
//     tPacket.reserve(sizeof(tHeader) + tBodySize);
//     tPacket.append(reinterpret_cast<const char*>(&tHeader), sizeof(tHeader));
//     tPacket.append(serializedData);

//     boost::asio::post(mStrand,
//     [self = shared_from_this(), pPacket = std::move(tPacket)]()
//     {
//         bool tWriteInProgress = !self->mWriteQueue.empty();
//         self->mWriteQueue.push_back(std::move(pPacket));

//         if (!tWriteInProgress)
//         {
//             self->doWrite();
//         }
//     });
// }

void Session::sendRequest(connection::Message& pMessage, ResponseCallback pCallback)
{
    uint64_t tRequestId = mNextRequestId++;

    pMessage.mutable_request()->mutable_header()->set_request_id(tRequestId);

    {
        std::lock_guard<std::mutex> tLock(mRequestMutex);
        mPendingRequests[tRequestId] = std::move(pCallback);
    }

    send(pMessage);
}

bool Session::isConnected() const
{
    return mSocket.is_open();
}

void Session::setMessageHandler(MessageHandler pHandler)
{
    mMessageHandler = pHandler;
}

void Session::setDisconnectHandler(DisconnectHandler pHandler)
{
    mDisconnectHandler = pHandler;
}

void Session::handleIncomingMessage(connection::Message& pMessage)
{
    uint64_t tRequestId = pMessage.mutable_response()->mutable_header()->request_id();

    if (tRequestId > 0)
    {
        ResponseCallback tCallback = nullptr;

        {
            std::lock_guard<std::mutex> tLock(mRequestMutex);

            auto tIterator = mPendingRequests.find(tRequestId);
            if (tIterator != mPendingRequests.end())
            {
                tCallback = std::move(tIterator->second);
                mPendingRequests.erase(tIterator);
            }
        }

        if (tCallback)
        {
            MainThreadExecutor::getInstance().enqueue(
            [tCallback, pMessage]()
            {
                tCallback(pMessage);
            });
            return;
        }
    }

    if (mMessageHandler)
    {
        mMessageHandler(shared_from_this(), pMessage);
    }
}

void Session::doReadHeader()
{
    boost::asio::async_read(mSocket,
        boost::asio::buffer(&mIncomingHeader, sizeof(mIncomingHeader)),
        boost::asio::bind_executor(mStrand,
    [self = shared_from_this()](boost::system::error_code pErrorCode, std::size_t /*length*/)
    {
        if (!pErrorCode)
        {
            // Network Order -> Host Order (Big Endian -> Little Endian)
            uint32_t bodySize = ntohl(self->mIncomingHeader);

            // GÜVENLİK KONTROLÜ (Allocation Bomb)
            if (bodySize > MAX_PACKET_SIZE)
            {
                std::cerr << "Packet size too large: " << bodySize << std::endl;
                self->close();
                return;
            }

            self->mIncomingBody.resize(bodySize);
            self->doReadBody();
        }
        else
        {
            self->close();
        }
    }));
}

void Session::doReadBody()
{
    boost::asio::async_read(mSocket,
        boost::asio::buffer(mIncomingBody),
        boost::asio::bind_executor(mStrand,
    [self = shared_from_this()](boost::system::error_code pErrorCode, std::size_t /*length*/)
    {
        if (!pErrorCode)
        {
            connection::Message msg;
            if (msg.ParseFromArray(self->mIncomingBody.data(), static_cast<int>(self->mIncomingBody.size())))
            {
                // Eğer handler atanmışsa çağır
                if (self->mMessageHandler)
                {
                    self->mMessageHandler(self, msg);
                }
            }
            else
            {
                std::cerr << "Protobuf parse error!" << std::endl;
            }

            self->doReadHeader();
        }
        else
        {
            self->close();
        }
    }));
}

// void Session::doWrite()
// {
//     if (mWriteQueue.empty())
//     {
//         return;
//     }

//     boost::asio::async_write(mSocket,
//         boost::asio::buffer(mWriteQueue.front()),
//         boost::asio::bind_executor(mStrand,
//     [self = shared_from_this()](boost::system::error_code pErrorCode, std::size_t /*length*/)
//     {
//         if (!pErrorCode)
//         {
//             self->mWriteQueue.pop_front();

//             if (!self->mWriteQueue.empty())
//             {
//                 self->doWrite();
//             }
//         }
//         else
//         {
//             std::cerr << "Write error: " << pErrorCode.message() << std::endl;
//             self->close();
//         }
//     }));
// }

void Session::send(const connection::Message& pMessage)
{
    std::string serializedData;
    if (!pMessage.SerializeToString(&serializedData)) {
        std::cerr << "[Session] Protobuf serialization failed!" << std::endl;
        return;
    }

    uint32_t tBodySize = static_cast<uint32_t>(serializedData.size());
    uint32_t tHeader = htonl(tBodySize);

    std::string tPacket;
    tPacket.reserve(sizeof(tHeader) + tBodySize);
    tPacket.append(reinterpret_cast<const char*>(&tHeader), sizeof(tHeader));
    tPacket.append(serializedData);

    // DEBUG LOG 1
    std::cout << "[Session] Send cagrildi. Paket boyutu: " << tPacket.size() << std::endl;

    boost::asio::post(mStrand,
    [self = shared_from_this(), pPacket = std::move(tPacket)]() mutable // <--- MUTABLE EKLENDI
    {
        bool tWriteInProgress = !self->mWriteQueue.empty();

        // DEBUG LOG 2
        std::cout << "[Session] Kuyruğa ekleniyor. Onceki durum: "
                  << (tWriteInProgress ? "Mesgul" : "Bosta") << std::endl;

        self->mWriteQueue.push_back(std::move(pPacket));

        if (!tWriteInProgress)
        {
            self->doWrite();
        }
    });
}

void Session::doWrite()
{
    if (mWriteQueue.empty()) return;

    // DEBUG LOG 3
    std::cout << "[Session] doWrite basladi. Yazilacak veri: "
              << mWriteQueue.front().size() << " byte." << std::endl;

    boost::asio::async_write(mSocket,
        boost::asio::buffer(mWriteQueue.front()),
        boost::asio::bind_executor(mStrand,
    [self = shared_from_this()](boost::system::error_code pErrorCode, std::size_t length)
    {
        if (!pErrorCode)
        {
            // DEBUG LOG 4
            std::cout << "[Session] Yazma basarili! (" << length << " byte)" << std::endl;

            self->mWriteQueue.pop_front();

            if (!self->mWriteQueue.empty())
            {
                self->doWrite();
            }
        }
        else
        {
            // DEBUG LOG 5 - KRITIK NOKTA
            std::cerr << "[Session] Yazma HATASI: " << pErrorCode.message()
                      << " (" << pErrorCode.value() << ")" << std::endl;
            self->close();
        }
    }));
}
