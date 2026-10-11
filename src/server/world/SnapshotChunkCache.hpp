#ifndef LAKOT_SERVER_SNAPSHOTCHUNKCACHE_H
#define LAKOT_SERVER_SNAPSHOTCHUNKCACHE_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "WireWriter.h"

namespace lakot
{

template <typename MessageType, int FieldNumber>
class SnapshotChunkCache
{
public:
    void clear()
    {
        for (uint32_t tIndex : mUsed)
        {
            mSlots[tIndex].offset = kEmpty;
        }

        mUsed.clear();
        mArena.clear();
    }

    template <typename Fill>
    void append(std::vector<char>& pOut, uint64_t pKey, Fill&& pFill)
    {
        if ((mUsed.size() + 1) * 2 > mSlots.size())
        {
            grow();
        }

        const size_t tMask = mSlots.size() - 1;
        size_t tIndex = hash(pKey) & tMask;

        while (mSlots[tIndex].offset != kEmpty && mSlots[tIndex].key != pKey)
        {
            tIndex = (tIndex + 1) & tMask;
        }

        Slot& tSlot = mSlots[tIndex];

        if (tSlot.offset == kEmpty)
        {
            tSlot.key = pKey;
            tSlot.length = 0;
            mUsed.push_back(static_cast<uint32_t>(tIndex));
            encode(tSlot, pFill);
        }

        if (tSlot.length != 0)
        {
            WireWriter::appendBytes(pOut, mArena.data() + tSlot.offset, tSlot.length);
        }
    }

private:
    static constexpr uint32_t kEmpty = UINT32_MAX;
    static constexpr size_t kInitialSlots = 256;

    struct Slot
    {
        uint64_t key = 0;
        uint32_t offset = kEmpty;
        uint32_t length = 0;
    };

    static size_t hash(uint64_t pKey)
    {
        return static_cast<size_t>((pKey * 0x9E3779B97F4A7C15ull) >> 32);
    }

    template <typename Fill>
    void encode(Slot& pSlot, Fill& pFill)
    {
        const size_t tStart = mArena.size();
        pSlot.offset = static_cast<uint32_t>(tStart);

        mScratch.Clear();

        if (pFill(mScratch))
        {
            const size_t tBodySize = mScratch.ByteSizeLong();
            WireWriter::appendLengthDelimitedHeader(mArena, FieldNumber, tBodySize);

            const size_t tBodyStart = mArena.size();
            mArena.resize(tBodyStart + tBodySize);
            mScratch.SerializeToArray(mArena.data() + tBodyStart, static_cast<int>(tBodySize));
        }

        pSlot.length = static_cast<uint32_t>(mArena.size() - tStart);
    }

    void grow()
    {
        std::vector<Slot> tOld = std::move(mSlots);
        std::vector<uint32_t> tOldUsed = std::move(mUsed);

        mSlots.assign(tOld.empty() ? kInitialSlots : tOld.size() * 2, Slot{});
        mUsed.clear();
        mUsed.reserve(tOldUsed.size() * 2);

        const size_t tMask = mSlots.size() - 1;

        for (uint32_t tOldIndex : tOldUsed)
        {
            const Slot& tOldSlot = tOld[tOldIndex];
            size_t tIndex = hash(tOldSlot.key) & tMask;

            while (mSlots[tIndex].offset != kEmpty)
            {
                tIndex = (tIndex + 1) & tMask;
            }

            mSlots[tIndex] = tOldSlot;
            mUsed.push_back(static_cast<uint32_t>(tIndex));
        }
    }

    std::vector<Slot> mSlots;
    std::vector<uint32_t> mUsed;
    std::vector<char> mArena;
    MessageType mScratch;
};

}

#endif
