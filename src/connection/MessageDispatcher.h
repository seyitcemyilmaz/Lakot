#ifndef LAKOT_MESSAGEDISPATCHER_H
#define LAKOT_MESSAGEDISPATCHER_H

#include <cstdint>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <syncstream>

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
    // uint32_t, matching generateKey()'s return type - a Response key has bit
    // 31 set, so an int-keyed map stored it as a negative value via an
    // implementation-defined conversion. Both sides converted identically so
    // lookups still worked, but the key type now says what it actually is.
    std::unordered_map<uint32_t, Handler> mHandlers;
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
            std::osyncstream(std::cerr) << "[Dispatcher] Unhandled Packet Key: " << pKey << " (ID: " << tOriginalId << ") from Account: " << pSession->getAccountId() << std::endl;
        }
    }
};

}

#endif
