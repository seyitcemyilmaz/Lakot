#include "LoginAttemptLimiter.h"

using namespace lakot;

bool LoginAttemptLimiter::isAllowed(const std::string& pAddress)
{
    if (pAddress.empty())
    {
        return true;
    }

    auto tNow = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> tLock(mMutex);

    auto tIterator = mAttempts.find(pAddress);

    if (tIterator == mAttempts.end())
    {
        return true;
    }

    if (tNow - tIterator->second.windowStart >= kAttemptWindow)
    {
        mAttempts.erase(tIterator);
        return true;
    }

    return tIterator->second.failedCount < kMaxFailedAttemptsPerWindow;
}

void LoginAttemptLimiter::recordFailure(const std::string& pAddress)
{
    if (pAddress.empty())
    {
        return;
    }

    auto tNow = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> tLock(mMutex);

    AttemptRecord& tRecord = mAttempts[pAddress];

    if (tRecord.failedCount == 0 || tNow - tRecord.windowStart >= kAttemptWindow)
    {
        tRecord.windowStart = tNow;
        tRecord.failedCount = 0;
    }

    tRecord.failedCount++;
}

void LoginAttemptLimiter::clear(const std::string& pAddress)
{
    if (pAddress.empty())
    {
        return;
    }

    std::lock_guard<std::mutex> tLock(mMutex);
    mAttempts.erase(pAddress);
}
