#include "SessionResumeManager.h"

#include <random>

using namespace lakot;

SessionResumeManager::SessionResumeManager(boost::asio::io_context& pContext)
    : mContext(pContext)
{

}

void SessionResumeManager::setReleaseCallback(ReleaseCallback pCallback)
{
    mReleaseCallback = std::move(pCallback);
}

std::string SessionResumeManager::issueToken(uint64_t pAccountId)
{
    uint64_t tReleasedCharacterId = 0;
    std::string tToken = generateToken();

    {
        std::lock_guard<std::mutex> tLock(mMutex);

        tReleasedCharacterId = eraseLocked(pAccountId);

        Entry& tEntry = mEntries[pAccountId];
        tEntry.token = tToken;
        mTokens[tToken] = pAccountId;
    }

    release(tReleasedCharacterId);

    return tToken;
}

void SessionResumeManager::setCharacter(uint64_t pAccountId, uint64_t pCharacterId)
{
    std::lock_guard<std::mutex> tLock(mMutex);

    auto tIterator = mEntries.find(pAccountId);

    if (tIterator != mEntries.end())
    {
        tIterator->second.characterId = pCharacterId;
    }
}

void SessionResumeManager::revoke(uint64_t pAccountId)
{
    std::lock_guard<std::mutex> tLock(mMutex);
    eraseLocked(pAccountId);
}

bool SessionResumeManager::hold(uint64_t pAccountId)
{
    std::lock_guard<std::mutex> tLock(mMutex);

    auto tIterator = mEntries.find(pAccountId);

    if (tIterator == mEntries.end())
    {
        return false;
    }

    Entry& tEntry = tIterator->second;

    tEntry.isHeld = true;
    tEntry.holdGeneration++;

    uint64_t tGeneration = tEntry.holdGeneration;

    tEntry.timer = std::make_shared<boost::asio::steady_timer>(mContext, kHoldDuration);
    tEntry.timer->async_wait(
    [this, pAccountId, tGeneration](const boost::system::error_code& pErrorCode)
    {
        if (!pErrorCode)
        {
            onHoldExpired(pAccountId, tGeneration);
        }
    });

    return true;
}

std::optional<SessionResumeManager::ResumeResult> SessionResumeManager::resume(const std::string& pToken)
{
    if (pToken.empty())
    {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> tLock(mMutex);

    auto tTokenIterator = mTokens.find(pToken);

    if (tTokenIterator == mTokens.end())
    {
        return std::nullopt;
    }

    uint64_t tAccountId = tTokenIterator->second;
    mTokens.erase(tTokenIterator);

    Entry& tEntry = mEntries[tAccountId];

    if (tEntry.timer)
    {
        tEntry.timer->cancel();
        tEntry.timer.reset();
    }

    tEntry.isHeld = false;
    tEntry.holdGeneration++; // a handler already queued must not release it

    tEntry.token = generateToken();
    mTokens[tEntry.token] = tAccountId;

    return ResumeResult{ tAccountId, tEntry.characterId, tEntry.token };
}

uint64_t SessionResumeManager::eraseLocked(uint64_t pAccountId)
{
    auto tIterator = mEntries.find(pAccountId);

    if (tIterator == mEntries.end())
    {
        return 0;
    }

    if (tIterator->second.timer)
    {
        tIterator->second.timer->cancel();
    }

    uint64_t tCharacterId = tIterator->second.characterId;

    mTokens.erase(tIterator->second.token);
    mEntries.erase(tIterator);

    return tCharacterId;
}

void SessionResumeManager::onHoldExpired(uint64_t pAccountId, uint64_t pGeneration)
{
    uint64_t tCharacterId = 0;

    {
        std::lock_guard<std::mutex> tLock(mMutex);

        auto tIterator = mEntries.find(pAccountId);

        if (tIterator == mEntries.end() || !tIterator->second.isHeld
            || tIterator->second.holdGeneration != pGeneration)
        {
            return; // resumed, re-issued or revoked in the meantime
        }

        tCharacterId = eraseLocked(pAccountId);
    }

    release(tCharacterId);
}

void SessionResumeManager::release(uint64_t pCharacterId)
{
    if (pCharacterId != 0 && mReleaseCallback)
    {
        mReleaseCallback(pCharacterId);
    }
}

std::string SessionResumeManager::generateToken()
{
    // 128 bits. std::random_device is the OS CSPRNG on the platforms this
    // builds for (rand_s on MSVC, /dev/urandom on Linux).
    static constexpr char kHexDigits[] = "0123456789abcdef";

    std::random_device tDevice;
    std::string tToken;
    tToken.reserve(32);

    for (int tWord = 0; tWord < 4; ++tWord)
    {
        uint32_t tValue = tDevice();

        for (int tNibble = 0; tNibble < 8; ++tNibble)
        {
            tToken.push_back(kHexDigits[(tValue >> (tNibble * 4)) & 0xF]);
        }
    }

    return tToken;
}
