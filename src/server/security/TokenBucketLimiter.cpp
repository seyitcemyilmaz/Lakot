#include "TokenBucketLimiter.h"

#include <algorithm>

using namespace lakot;

TokenBucketLimiter::TokenBucketLimiter(double pCapacity, double pTokensPerSecond)
    : mCapacity(pCapacity)
    , mTokensPerSecond(pTokensPerSecond)
{

}

bool TokenBucketLimiter::tryConsume(uint64_t pKey, double pCost)
{
    auto tNow = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> tLock(mMutex);

    if (mBuckets.size() >= kPruneThreshold)
    {
        pruneIdle(tNow);
    }

    auto [tIterator, tIsNew] = mBuckets.try_emplace(pKey);
    Bucket& tBucket = tIterator->second;

    if (tIsNew)
    {
        tBucket.tokens = mCapacity;
    }
    else
    {
        double tElapsedSeconds = std::chrono::duration<double>(tNow - tBucket.lastRefill).count();
        tBucket.tokens = std::min(mCapacity, tBucket.tokens + tElapsedSeconds * mTokensPerSecond);
    }

    tBucket.lastRefill = tNow;

    if (tBucket.tokens < pCost)
    {
        return false;
    }

    tBucket.tokens -= pCost;
    return true;
}

void TokenBucketLimiter::pruneIdle(std::chrono::steady_clock::time_point pNow)
{
    double tRefillSeconds = mTokensPerSecond > 0.0 ? mCapacity / mTokensPerSecond : 0.0;

    for (auto tIterator = mBuckets.begin(); tIterator != mBuckets.end();)
    {
        double tIdleSeconds = std::chrono::duration<double>(pNow - tIterator->second.lastRefill).count();

        if (tIdleSeconds >= tRefillSeconds)
        {
            tIterator = mBuckets.erase(tIterator);
        }
        else
        {
            ++tIterator;
        }
    }
}
