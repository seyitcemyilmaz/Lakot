#ifndef LAKOT_SESSIONREGISTRY_H
#define LAKOT_SESSIONREGISTRY_H

#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include <vector>

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

    // Returns true only if pSession was actually the registered session for
    // pUserId and got removed. False means this id has since been taken over
    // by a NEWER session (a relogin) and nothing was touched - callers must
    // treat that as "this user is still online" and skip any disconnect
    // cleanup, or they tear down the live session's state instead.
    bool remove(uint64_t pUserId, std::shared_ptr<SessionType> pSession)
    {
        std::unique_lock tLock(mMutex);

        auto tIterator = mSessions.find(pUserId);

        if (tIterator == mSessions.end())
        {
            return false;
        }

        if (tIterator->second != pSession)
        {
            // Demek ki bu ID'ye sahip başka (yeni) bir session gelmiş.
            // Dokunma! (Race condition önlendi)
            return false;
        }

        mSessions.erase(tIterator);
        return true;
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

    // ---- Character index ----
    //
    // The account index above answers "is this account already online" (which
    // is what the relogin kick needs). This second one answers "which session
    // is this world entity", which is what everything gameplay-side actually
    // asks: a zone knows character ids, a whisper resolves to a character id.
    //
    // They are separate because they have different lifetimes. A session is
    // account-registered from login until disconnect, but character-bound
    // only from entering the world - an authenticated player sitting on the
    // character selection screen belongs in the first index and must NOT
    // appear in the second, or world traffic would be routed to someone who
    // is not in the world.

    void bindCharacter(uint64_t pCharacterId, std::shared_ptr<SessionType> pSession)
    {
        std::unique_lock tLock(mMutex);
        mCharacterSessions[pCharacterId] = std::move(pSession);
    }

    // Same identity guard as remove(): only clears the binding if pSession is
    // still the session bound to that character, so a superseded connection
    // cannot unbind a live one.
    void unbindCharacter(uint64_t pCharacterId, const std::shared_ptr<SessionType>& pSession)
    {
        std::unique_lock tLock(mMutex);

        auto tIterator = mCharacterSessions.find(pCharacterId);

        if (tIterator != mCharacterSessions.end() && tIterator->second == pSession)
        {
            mCharacterSessions.erase(tIterator);
        }
    }

    std::shared_ptr<SessionType> getByCharacter(uint64_t pCharacterId) const
    {
        std::shared_lock tLock(mMutex);

        auto tIterator = mCharacterSessions.find(pCharacterId);

        if (tIterator == mCharacterSessions.end())
        {
            return nullptr;
        }

        return tIterator->second;
    }

    // Every character currently IN THE WORLD - e.g. for a server-wide chat
    // broadcast that still needs to exclude the sender, which broadcast()
    // can't express since it has no exclusion. Players still at character
    // select are deliberately not included.
    std::vector<uint64_t> getAllCharacterIds() const
    {
        std::shared_lock tLock(mMutex);

        std::vector<uint64_t> tIds;
        tIds.reserve(mCharacterSessions.size());

        for (const auto& [tId, tSession] : mCharacterSessions)
        {
            tIds.push_back(tId);
        }

        return tIds;
    }

private:
    mutable std::shared_mutex mMutex;

    std::unordered_map<uint64_t, std::shared_ptr<SessionType>> mSessions;          // accountId  -> session
    std::unordered_map<uint64_t, std::shared_ptr<SessionType>> mCharacterSessions; // characterId -> session
};

}

#endif
