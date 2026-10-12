#ifndef LAKOT_SERVER_LOGINATTEMPTLIMITER_H
#define LAKOT_SERVER_LOGINATTEMPTLIMITER_H

#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>

#include "AttemptTable.h"

namespace lakot
{

class LoginAttemptLimiter
{
public:
    bool tryAcquire(const std::string& pAddress, const std::string& pUsername);

    void recordFailure(const std::string& pAddress, const std::string& pUsername);
    void recordSuccess(const std::string& pAddress, const std::string& pUsername);
    void release(const std::string& pAddress, const std::string& pUsername);
    void recordMalformed(const std::string& pAddress);

private:
    static constexpr int kMaxFailuresPerAddress = 8;
    static constexpr int kMaxFailuresPerPair = 5;
    static constexpr int kMaxFailuresPerUsername = 20;
    static constexpr std::chrono::seconds kAddressWindow{60};
    static constexpr std::chrono::seconds kPairWindow{60};
    static constexpr std::chrono::seconds kUsernameWindow{300};
    static constexpr size_t kSweepBucketsPerCall = 32;

    std::mutex mMutex;
    AttemptTable mAddressTable{kMaxFailuresPerAddress, kAddressWindow};
    AttemptTable mPairTable{kMaxFailuresPerPair, kPairWindow};
    AttemptTable mUsernameTable{kMaxFailuresPerUsername, kUsernameWindow};

    static std::string makePairKey(const std::string& pAddress, const std::string& pUsername);
    void sweepStep(std::chrono::steady_clock::time_point pNow);
};

}

#endif
