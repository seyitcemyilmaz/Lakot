#ifndef LAKOT_SERVER_ATTEMPTTABLE_H
#define LAKOT_SERVER_ATTEMPTTABLE_H

#include <chrono>
#include <cstddef>
#include <string>
#include <unordered_map>

namespace lakot
{

class AttemptTable
{
public:
    AttemptTable(int pMaxFailures, std::chrono::seconds pWindow);

    bool hasRoom(const std::string& pKey, std::chrono::steady_clock::time_point pNow);
    void reserve(const std::string& pKey, std::chrono::steady_clock::time_point pNow);
    void release(const std::string& pKey);
    void addFailure(const std::string& pKey, std::chrono::steady_clock::time_point pNow);
    void reset(const std::string& pKey);
    void sweep(std::chrono::steady_clock::time_point pNow, size_t pBucketBudget);

private:
    struct AttemptRecord
    {
        int failedCount = 0;
        int pendingCount = 0;
        std::chrono::steady_clock::time_point windowStart;
        std::chrono::steady_clock::time_point pendingSince;
    };

    static constexpr std::chrono::seconds kPendingTimeout{30};
    static constexpr size_t kMaxRecords = 50000;
    static constexpr size_t kFullSweepBuckets = 2048;

    int mMaxFailures;
    std::chrono::seconds mWindow;
    std::unordered_map<std::string, AttemptRecord> mRecords;
    size_t mSweepCursor = 0;

    void expire(AttemptRecord& pRecord, std::chrono::steady_clock::time_point pNow) const;
    static bool isIdle(const AttemptRecord& pRecord);
};

}

#endif
