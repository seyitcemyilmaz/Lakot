#ifndef LAKOT_SERVER_SNAPSHOTENCODER_H
#define LAKOT_SERVER_SNAPSHOTENCODER_H

#include <cstdint>
#include <memory>
#include <vector>

#include <connection.pb.h>

#include "SnapshotChunkCache.hpp"

namespace lakot
{

class SnapshotEncoder
{
public:
    using Packet = std::shared_ptr<const std::vector<char>>;

    void beginRound();
    void beginMessage(uint32_t pTick);

    template <typename Fill>
    void addEntered(uint64_t pEntityId, Fill&& pFill)
    {
        mEntered.append(mBody, pEntityId, pFill);
    }

    template <typename Fill>
    void addMoved(uint64_t pEntityId, Fill&& pFill)
    {
        mMoved.append(mBody, pEntityId, pFill);
    }

    void addLeft(const std::vector<uint64_t>& pEntityIds);
    void setSelfCorrection(const services::world::EntityState& pState);

    template <typename Fill>
    void addAppearance(uint64_t pEntityId, Fill&& pFill)
    {
        mAppearance.append(mBody, pEntityId, pFill);
    }

    template <typename Fill>
    void addCombat(uint64_t pRecordIndex, Fill&& pFill)
    {
        mCombat.append(mBody, pRecordIndex, pFill);
    }

    template <typename Fill>
    void addVitals(uint64_t pEntityId, Fill&& pFill)
    {
        mVitals.append(mBody, pEntityId, pFill);
    }

    void setSelfVitals(const services::world::SelfVitals& pVitals);

    Packet finishMessage();

private:
    using WorldSnapshot = services::world::WorldSnapshot;

    template <typename MessageType>
    void appendMessage(int pFieldNumber, const MessageType& pMessage)
    {
        const size_t tSize = pMessage.ByteSizeLong();
        WireWriter::appendLengthDelimitedHeader(mBody, pFieldNumber, tSize);

        const size_t tStart = mBody.size();
        mBody.resize(tStart + tSize);
        pMessage.SerializeToArray(mBody.data() + tStart, static_cast<int>(tSize));
    }

    SnapshotChunkCache<services::world::EntitySpawn, WorldSnapshot::kEnteredFieldNumber> mEntered;
    SnapshotChunkCache<services::world::EntityState, WorldSnapshot::kMovedFieldNumber> mMoved;
    SnapshotChunkCache<services::world::EntityAppearance, WorldSnapshot::kAppearanceFieldNumber> mAppearance;
    SnapshotChunkCache<services::world::CombatEvent, WorldSnapshot::kCombatFieldNumber> mCombat;
    SnapshotChunkCache<services::world::EntityVitals, WorldSnapshot::kVitalsFieldNumber> mVitals;

    std::vector<char> mBody;
};

}

#endif
