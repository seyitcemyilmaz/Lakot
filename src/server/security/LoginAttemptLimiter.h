#ifndef LAKOT_SERVER_LOGINATTEMPTLIMITER_H
#define LAKOT_SERVER_LOGINATTEMPTLIMITER_H

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

namespace lakot
{

class LoginAttemptLimiter
{
public:
    bool isAllowed(const std::string& pAddress);

    void recordFailure(const std::string& pAddress);
    void clear(const std::string& pAddress);

private:
    static constexpr int kMaxFailedAttemptsPerWindow = 8;
    static constexpr std::chrono::seconds kAttemptWindow{60};

    struct AttemptRecord
    {
        int failedCount = 0;
        std::chrono::steady_clock::time_point windowStart;
    };

    std::mutex mMutex;
    std::unordered_map<std::string, AttemptRecord> mAttempts;
};

}

#endif
