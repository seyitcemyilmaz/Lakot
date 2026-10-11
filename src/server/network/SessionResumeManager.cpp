#include "SessionResumeManager.h"

#include <random>

using namespace lakot;

SessionResumeManager::SessionResumeManager(boost::asio::io_context& pContext, ExpiryCallback pExpiryCallback)
    : mContext(pContext)
    , mExpiryCallback(std::move(pExpiryCallback))
{

}

SessionResumeManager::IssueResult SessionResumeManager::issueToken(uint64_t pAccountId)
{
    IssueResult tResult;
    tResult.token = generateToken();

    std::lock_guard<std::mutex> tLock(mMutex);

    tResult.releasedCharacterId = eraseLocked(pAccountId);

    Entry& tEntry = mEntries[pAccountId];
    tEntry.token = tResult.token;
    mTokens[tResult.token] = pAccountId;

    return tResult;
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

uint64_t SessionResumeManager::revoke(uint64_t pAccountId)
{
    std::lock_guard<std::mutex> tLock(mMutex);
    return eraseLocked(pAccountId);
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
        if (!pErrorCode && mExpiryCallback)
        {
            mExpiryCallback(pAccountId, tGeneration);
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

uint64_t SessionResumeManager::expireHold(uint64_t pAccountId, uint64_t pGeneration)
{
    std::lock_guard<std::mutex> tLock(mMutex);

    auto tIterator = mEntries.find(pAccountId);

    if (tIterator == mEntries.end() || !tIterator->second.isHeld
        || tIterator->second.holdGeneration != pGeneration)
    {
        return 0;
    }

    return eraseLocked(pAccountId);
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
