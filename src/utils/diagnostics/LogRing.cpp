#include "diagnostics/LogRing.h"

#include <algorithm>
#include <chrono>
#include <thread>

using namespace lakot;

void LogRing::append(std::string_view pText)
{
    if (pText.empty())
    {
        return;
    }

    if (pText.size() > kCapacity)
    {
        pText.remove_prefix(pText.size() - kCapacity);
    }

    std::lock_guard<std::mutex> tLock(mMutex);

    std::uint64_t tTotal = mTotal.load(std::memory_order_relaxed);
    std::size_t tPosition = static_cast<std::size_t>(tTotal % kCapacity);
    const std::size_t tFirst = std::min(pText.size(), kCapacity - tPosition);

    std::copy_n(pText.data(), tFirst, mData + tPosition);
    std::copy_n(pText.data() + tFirst, pText.size() - tFirst, mData);

    mTotal.store(tTotal + pText.size(), std::memory_order_release);
}

std::size_t LogRing::copyTail(char* pOut, std::size_t pCapacity, std::size_t pMaxLines, unsigned pLockWaitMilliseconds)
{
    if (pCapacity == 0)
    {
        return 0;
    }

    bool tLocked = mMutex.try_lock();

    for (unsigned tWaited = 0; !tLocked && tWaited < pLockWaitMilliseconds; ++tWaited)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        tLocked = mMutex.try_lock();
    }

    const std::uint64_t tTotal = mTotal.load(std::memory_order_acquire);
    const std::size_t tAvailable = static_cast<std::size_t>(std::min<std::uint64_t>(tTotal, kCapacity));
    const std::uint64_t tOrigin = tTotal - tAvailable;
    std::size_t tStart = 0;
    std::size_t tLines = 0;

    for (std::size_t tPosition = tAvailable; tPosition > 0; --tPosition)
    {
        const std::size_t tOffset = tPosition - 1;

        if (mData[(tOrigin + tOffset) % kCapacity] == '\n' && tPosition != tAvailable && ++tLines >= pMaxLines)
        {
            tStart = tPosition;
            break;
        }
    }

    std::size_t tLength = tAvailable - tStart;

    if (tLength > pCapacity - 1)
    {
        tStart += tLength - (pCapacity - 1);
        tLength = pCapacity - 1;
    }

    const std::uint64_t tBegin = tOrigin + tStart;

    for (std::size_t tIndex = 0; tIndex < tLength; ++tIndex)
    {
        pOut[tIndex] = mData[(tBegin + tIndex) % kCapacity];
    }

    pOut[tLength] = '\0';

    if (tLocked)
    {
        mMutex.unlock();
    }

    return tLength;
}
