#include "AttemptTable.h"

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
        return true;
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

void AttemptTable::sweep(std::chrono::steady_clock::time_point pNow)
{
    for (auto tIterator = mRecords.begin(); tIterator != mRecords.end();)
    {
        expire(tIterator->second, pNow);

        if (isIdle(tIterator->second))
        {
            tIterator = mRecords.erase(tIterator);
        }
        else
        {
            ++tIterator;
        }
    }
}

void AttemptTable::expire(AttemptRecord& pRecord, std::chrono::steady_clock::time_point pNow) const
{
    if (pRecord.failedCount > 0 && pNow - pRecord.windowStart >= mWindow)
    {
        pRecord.failedCount = 0;
    }
}

bool AttemptTable::isIdle(const AttemptRecord& pRecord)
{
    return pRecord.failedCount == 0 && pRecord.pendingCount == 0;
}
