#ifndef LAKOT_MESSAGEDISPATCHER_H
#define LAKOT_MESSAGEDISPATCHER_H

#include <iostream>

#include "NetworkSession.h"

namespace lakot
{

enum class PacketType
{
    Request = 0,
    Response = 1
};

template <typename MessageType>
class LAKOT_CONNECTION_EXPORT MessageDispatcher
{
public:
    using SessionPtr = std::shared_ptr<NetworkSession<MessageType>>;
    using Handler = std::function<void(SessionPtr, const MessageType&)>;
    using KeyExtractor = std::function<uint32_t(const MessageType&)>;

    virtual ~MessageDispatcher() = default;
    MessageDispatcher() = default;


    void registerHandler(int pCaseId, PacketType pType, Handler pHandler)
    {
        uint32_t tKey = generateKey(pCaseId, pType);
        mHandlers[tKey] = std::move(pHandler);
    }

    void dispatch(SessionPtr pSession, const MessageType& pMessage, KeyExtractor pKeyExtractor)
    {
        uint32_t tKey = pKeyExtractor(pMessage);

        auto tIterator = mHandlers.find(tKey);

        if (tIterator != mHandlers.end())
        {
            tIterator->second(pSession, pMessage);
        }
        else
        {
            handleUnknownPacket(pSession, tKey);
        }
    }

    void setUnknownPacketHandler(std::function<void(SessionPtr, int)> pHandler)
    {
        mUnknownPacketHandler = pHandler;
    }

    static uint32_t generateKey(int pId, PacketType pType)
    {
        return static_cast<uint32_t>(pId) | (pType == PacketType::Response ? 0x80000000 : 0);
    }

private:
    std::unordered_map<int, Handler> mHandlers;
    std::function<void(SessionPtr, int)> mUnknownPacketHandler;

    void handleUnknownPacket(SessionPtr pSession, uint32_t pKey)
    {
        int tOriginalId = static_cast<int>(pKey & 0x7FFFFFFF);

        if (mUnknownPacketHandler)
        {
            mUnknownPacketHandler(pSession, tOriginalId);
        }
        else
        {
            std::cerr << "[Dispatcher] Unhandled Packet Key: " << pKey << " (ID: " << tOriginalId << ") from User: " << pSession->getUserId() << std::endl;
        }
    }
};

}

#endif
