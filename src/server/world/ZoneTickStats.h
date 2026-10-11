#ifndef LAKOT_SERVER_ZONETICKSTATS_H
#define LAKOT_SERVER_ZONETICKSTATS_H

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace lakot
{

class ZoneTickStats
{
public:
    using Clock = std::chrono::steady_clock;

    enum class SectionType
    {
        ePortal,
        eMonster,
        eCombatLoot,
        eDeltas,
        eBuildSend,
        eCount
    };

    ZoneTickStats(uint32_t pMapId, std::chrono::milliseconds pTickInterval);

    bool isQueueWaitSampled();

    void beginTick();
    void beginSection();
    void endSection(SectionType pSection);
    void recordPacket(size_t pBytes);
    void recordQueueWait(Clock::duration pWait);
    void endTick(size_t pPlayerCount);

private:
    static constexpr uint32_t kReportEveryTicks = 100;
    static constexpr uint32_t kQueueSampleMask = 15;

    struct DurationStat
    {
        Clock::duration sum{};
        Clock::duration max{};
        uint32_t count = 0;

        void add(Clock::duration pValue);
    };

    void report(size_t pPlayerCount) const;
    void reset();

    uint32_t mMapId;
    Clock::duration mTickInterval;

    std::atomic<uint32_t> mPostCounter{0};

    Clock::time_point mLastTickStart{};
    Clock::time_point mTickStart{};
    Clock::time_point mSectionStart{};

    uint32_t mTicks = 0;
    uint32_t mOverruns = 0;
    DurationStat mPeriod;
    DurationStat mTickDuration;
    std::array<DurationStat, static_cast<size_t>(SectionType::eCount)> mSections;
    DurationStat mQueueWait;

    uint64_t mSnapshotBytes = 0;
    uint32_t mSnapshotPackets = 0;
    size_t mMaxPacketBytes = 0;
};

}

#endif
