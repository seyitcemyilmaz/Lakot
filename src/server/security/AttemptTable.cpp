#include "AttemptTable.h"

#include <vector>

using namespace lakot;

AttemptTable::AttemptTable(int pMaxFailures, std::chrono::seconds pWindow)
    : mMaxFailures(pMaxFailures)
    , mWindow(pWindow)
{

}

bool AttemptTable::hasRoom(const std::string& pKey, std::chrono::steady_clock::time_point pNow)
{
    auto tIterator = mRecords.find(pKey);

    if (tIterator == mRecords.end())
    {
        if (mRecords.size() < kMaxRecords)
        {
            return true;
        }

        sweep(pNow, kFullSweepBuckets);

        return mRecords.size() < kMaxRecords;
    }

    expire(tIterator->second, pNow);

    if (isIdle(tIterator->second))
    {
        mRecords.erase(tIterator);
        return true;
    }

    return tIterator->second.failedCount + tIterator->second.pendingCount < mMaxFailures;
}

void AttemptTable::reserve(const std::string& pKey, std::chrono::steady_clock::time_point pNow)
{
    AttemptRecord& tRecord = mRecords[pKey];

    expire(tRecord, pNow);

    if (tRecord.pendingCount == 0)
    {
        tRecord.pendingSince = pNow;
    }

    tRecord.pendingCount++;
}

void AttemptTable::release(const std::string& pKey)
{
    auto tIterator = mRecords.find(pKey);

    if (tIterator == mRecords.end())
    {
        return;
    }

    if (tIterator->second.pendingCount > 0)
    {
        tIterator->second.pendingCount--;
    }

    if (isIdle(tIterator->second))
    {
        mRecords.erase(tIterator);
    }
}

void AttemptTable::addFailure(const std::string& pKey, std::chrono::steady_clock::time_point pNow)
{
    if (mRecords.size() >= kMaxRecords && mRecords.find(pKey) == mRecords.end())
    {
        return;
    }

    AttemptRecord& tRecord = mRecords[pKey];

    expire(tRecord, pNow);

    if (tRecord.failedCount == 0)
    {
        tRecord.windowStart = pNow;
    }

    tRecord.failedCount++;
}

void AttemptTable::reset(const std::string& pKey)
{
    auto tIterator = mRecords.find(pKey);

    if (tIterator == mRecords.end())
    {
        return;
    }

    tIterator->second.failedCount = 0;

    if (isIdle(tIterator->second))
    {
        mRecords.erase(tIterator);
    }
}

void AttemptTable::sweep(std::chrono::steady_clock::time_point pNow, size_t pBucketBudget)
{
    size_t tBucketCount = mRecords.bucket_count();

    if (tBucketCount == 0)
    {
        return;
    }

    size_t tBuckets = pBucketBudget < tBucketCount ? pBucketBudget : tBucketCount;
    std::vector<std::string> tExpired;

    for (size_t tIndex = 0; tIndex < tBuckets; ++tIndex)
    {
        size_t tBucket = (mSweepCursor + tIndex) % tBucketCount;

        for (auto tIterator = mRecords.begin(tBucket); tIterator != mRecords.end(tBucket); ++tIterator)
        {
            expire(tIterator->second, pNow);

            if (isIdle(tIterator->second))
            {
                tExpired.push_back(tIterator->first);
            }
        }
    }

    mSweepCursor = (mSweepCursor + tBuckets) % tBucketCount;

    for (const std::string& tKey : tExpired)
    {
        mRecords.erase(tKey);
    }
}

void AttemptTable::expire(AttemptRecord& pRecord, std::chrono::steady_clock::time_point pNow) const
{
    if (pRecord.failedCount > 0 && pNow - pRecord.windowStart >= mWindow)
    {
        pRecord.failedCount = 0;
    }

    if (pRecord.pendingCount > 0 && pNow - pRecord.pendingSince >= kPendingTimeout)
    {
        pRecord.pendingCount = 0;
    }
}

bool AttemptTable::isIdle(const AttemptRecord& pRecord)
{
    return pRecord.failedCount == 0 && pRecord.pendingCount == 0;
}
