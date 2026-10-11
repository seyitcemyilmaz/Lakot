#include "ZoneTickStats.h"

#include <algorithm>
#include <format>
#include <iostream>
#include <string>

using namespace lakot;

namespace
{

double toMilliseconds(ZoneTickStats::Clock::duration pValue)
{
    return std::chrono::duration<double, std::milli>(pValue).count();
}

}

void ZoneTickStats::DurationStat::add(Clock::duration pValue)
{
    sum += pValue;
    max = std::max(max, pValue);
    ++count;
}

ZoneTickStats::ZoneTickStats(uint32_t pMapId, std::chrono::milliseconds pTickInterval)
    : mMapId(pMapId)
    , mTickInterval(pTickInterval)
{
}

bool ZoneTickStats::isQueueWaitSampled()
{
    return (mPostCounter.fetch_add(1, std::memory_order_relaxed) & kQueueSampleMask) == 0;
}

void ZoneTickStats::beginTick()
{
    mTickStart = Clock::now();

    if (mLastTickStart != Clock::time_point{})
    {
        mPeriod.add(mTickStart - mLastTickStart);
    }

    mLastTickStart = mTickStart;
}

void ZoneTickStats::beginSection()
{
    mSectionStart = Clock::now();
}

void ZoneTickStats::endSection(SectionType pSection)
{
    mSections[static_cast<size_t>(pSection)].add(Clock::now() - mSectionStart);
}

void ZoneTickStats::recordPacket(size_t pBytes)
{
    mSnapshotBytes += pBytes;
    ++mSnapshotPackets;
    mMaxPacketBytes = std::max(mMaxPacketBytes, pBytes);
}

void ZoneTickStats::recordQueueWait(Clock::duration pWait)
{
    mQueueWait.add(pWait);
}

void ZoneTickStats::endTick(size_t pPlayerCount)
{
    Clock::duration tDuration = Clock::now() - mTickStart;
    mTickDuration.add(tDuration);

    if (tDuration > mTickInterval)
    {
        ++mOverruns;
    }

    if (++mTicks < kReportEveryTicks)
    {
        return;
    }

    if (pPlayerCount > 0)
    {
        report(pPlayerCount);
    }

    reset();
}

void ZoneTickStats::report(size_t pPlayerCount) const
{
    static constexpr const char* kSectionNames[] = { "portal", "monster", "combat", "deltas", "send" };

    std::string tLine = std::format("[ZoneStats] map={} players={} ticks={} period avg={:.1f} max={:.1f} ms, tick avg={:.2f} max={:.2f} ms, overruns={}",
        mMapId, pPlayerCount, mTicks,
        mPeriod.count ? toMilliseconds(mPeriod.sum) / mPeriod.count : 0.0, toMilliseconds(mPeriod.max),
        mTickDuration.count ? toMilliseconds(mTickDuration.sum) / mTickDuration.count : 0.0, toMilliseconds(mTickDuration.max),
        mOverruns);

    for (size_t tIndex = 0; tIndex < mSections.size(); ++tIndex)
    {
        const DurationStat& tSection = mSections[tIndex];
        tLine += std::format(" | {} avg={:.2f} max={:.2f}", kSectionNames[tIndex],
            tSection.count ? toMilliseconds(tSection.sum) / tSection.count : 0.0, toMilliseconds(tSection.max));
    }

    tLine += std::format(" | snapshots packets={} bytes={} maxViewerBytes={}", mSnapshotPackets, mSnapshotBytes, mMaxPacketBytes);
    tLine += std::format(" | queueWait samples={} avg={:.2f} max={:.2f} ms", mQueueWait.count,
        mQueueWait.count ? toMilliseconds(mQueueWait.sum) / mQueueWait.count : 0.0, toMilliseconds(mQueueWait.max));

    std::cout << tLine << std::endl;
}

void ZoneTickStats::reset()
{
    mTicks = 0;
    mOverruns = 0;
    mPeriod = DurationStat{};
    mTickDuration = DurationStat{};
    mSections = {};
    mQueueWait = DurationStat{};
    mSnapshotBytes = 0;
    mSnapshotPackets = 0;
    mMaxPacketBytes = 0;
}
