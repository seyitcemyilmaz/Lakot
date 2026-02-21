#ifndef LAKOT_SESSIONREGISTRY_H
#define LAKOT_SESSIONREGISTRY_H

#include <unordered_map>
#include <shared_mutex>
#include <memory>

#include "connection_global.h"

namespace lakot
{

template <typename SessionType>
class LAKOT_CONNECTION_EXPORT SessionRegistry
{
public:
    virtual ~SessionRegistry() = default;
    SessionRegistry() = default;

    std::shared_ptr<SessionType> addOrReplace(uint64_t pUserId, std::shared_ptr<SessionType> pSession)
    {
        std::unique_lock tLock(mMutex);

        std::shared_ptr<SessionType> tOldSession = nullptr;

        auto tIterator = mSessions.find(pUserId);

        if (tIterator != mSessions.end())
        {
            tOldSession = tIterator->second;
            tIterator->second = pSession;
        }
        else
        {
            mSessions[pUserId] = pSession;
        }

        return tOldSession;
    }

    void remove(uint64_t pUserId, std::shared_ptr<SessionType> pSession)
    {
        std::unique_lock tLock(mMutex);

        auto tIterator = mSessions.find(pUserId);

        if (tIterator != mSessions.end())
        {
            if (tIterator->second == pSession)
            {
                mSessions.erase(tIterator);
            }
            else
            {
                // Demek ki bu ID'ye sahip başka (yeni) bir session gelmiş.
                // Dokunma! (Race condition önlendi)
            }
        }
    }

    std::shared_ptr<SessionType> get(uint64_t pUserId)
    {
        std::shared_lock tLock(mMutex);

        auto tIterator = mSessions.find(pUserId);

        if (tIterator == mSessions.end())
        {
            return nullptr;
        }

        return tIterator->second;
    }

    size_t size()
    {
        std::shared_lock tLock(mMutex);
        return mSessions.size();
    }

    template <typename MessageType>
    void broadcast(const MessageType& pMessage)
    {
        std::shared_lock tLock(mMutex);

        for (const auto& [tId, tSession] : mSessions)
        {
            tSession->send(pMessage);
        }
    }

private:
    std::shared_mutex mMutex;
    std::unordered_map<uint64_t, std::shared_ptr<SessionType>> mSessions;
};

}

#endif
