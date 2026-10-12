#include "LoginAttemptLimiter.h"

using namespace lakot;

bool LoginAttemptLimiter::tryAcquire(const std::string& pAddress, const std::string& pUsername)
{
    auto tNow = std::chrono::steady_clock::now();
    bool tHasAddress = !pAddress.empty();
    std::string tPairKey = makePairKey(pAddress, pUsername);

    std::lock_guard<std::mutex> tLock(mMutex);

    sweepStep(tNow);

    if (tHasAddress && (!mAddressTable.hasRoom(pAddress, tNow) || !mPairTable.hasRoom(tPairKey, tNow)))
    {
        return false;
    }

    if (!mUsernameTable.hasRoom(pUsername, tNow))
    {
        return false;
    }

    if (tHasAddress)
    {
        mAddressTable.reserve(pAddress, tNow);
        mPairTable.reserve(tPairKey, tNow);
    }

    mUsernameTable.reserve(pUsername, tNow);
    return true;
}

void LoginAttemptLimiter::recordFailure(const std::string& pAddress, const std::string& pUsername)
{
    auto tNow = std::chrono::steady_clock::now();
    std::string tPairKey = makePairKey(pAddress, pUsername);

    std::lock_guard<std::mutex> tLock(mMutex);

    if (!pAddress.empty())
    {
        mAddressTable.release(pAddress);
        mAddressTable.addFailure(pAddress, tNow);
        mPairTable.release(tPairKey);
        mPairTable.addFailure(tPairKey, tNow);
    }

    mUsernameTable.release(pUsername);
    mUsernameTable.addFailure(pUsername, tNow);
}

void LoginAttemptLimiter::recordSuccess(const std::string& pAddress, const std::string& pUsername)
{
    std::string tPairKey = makePairKey(pAddress, pUsername);

    std::lock_guard<std::mutex> tLock(mMutex);

    if (!pAddress.empty())
    {
        mAddressTable.release(pAddress);
        mPairTable.release(tPairKey);
        mPairTable.reset(tPairKey);
    }

    mUsernameTable.release(pUsername);
}

void LoginAttemptLimiter::release(const std::string& pAddress, const std::string& pUsername)
{
    std::string tPairKey = makePairKey(pAddress, pUsername);

    std::lock_guard<std::mutex> tLock(mMutex);

    if (!pAddress.empty())
    {
        mAddressTable.release(pAddress);
        mPairTable.release(tPairKey);
    }

    mUsernameTable.release(pUsername);
}

void LoginAttemptLimiter::recordMalformed(const std::string& pAddress)
{
    if (pAddress.empty())
    {
        return;
    }

    auto tNow = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> tLock(mMutex);

    sweepStep(tNow);
    mAddressTable.addFailure(pAddress, tNow);
}

std::string LoginAttemptLimiter::makePairKey(const std::string& pAddress, const std::string& pUsername)
{
    return pAddress + '|' + pUsername;
}

void LoginAttemptLimiter::sweepStep(std::chrono::steady_clock::time_point pNow)
{
    mAddressTable.sweep(pNow, kSweepBucketsPerCall);
    mPairTable.sweep(pNow, kSweepBucketsPerCall);
    mUsernameTable.sweep(pNow, kSweepBucketsPerCall);
}
