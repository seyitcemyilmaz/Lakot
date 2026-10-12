#ifndef LAKOT_SERVER_TOKENBUCKETLIMITER_H
#define LAKOT_SERVER_TOKENBUCKETLIMITER_H

#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace lakot
{

class TokenBucketLimiter
{
public:
    TokenBucketLimiter(double pCapacity, double pTokensPerSecond);

    bool tryConsume(uint64_t pKey, double pCost = 1.0);

private:
    static constexpr size_t kPruneThreshold = 4096;
    static constexpr std::chrono::seconds kPruneInterval{1};

    struct Bucket
    {
        double tokens = 0.0;
        std::chrono::steady_clock::time_point lastRefill;
    };

    const double mCapacity;
    const double mTokensPerSecond;

    std::mutex mMutex;
    std::unordered_map<uint64_t, Bucket> mBuckets;
    std::chrono::steady_clock::time_point mNextPruneAt{};

    void pruneIdle(std::chrono::steady_clock::time_point pNow);
};

}

#endif
