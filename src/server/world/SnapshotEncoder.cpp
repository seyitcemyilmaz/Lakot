#include "SnapshotEncoder.h"

using namespace lakot;

void SnapshotEncoder::beginRound()
{
    mEntered.clear();
    mMoved.clear();
    mAppearance.clear();
    mCombat.clear();
    mVitals.clear();
}

void SnapshotEncoder::beginMessage(uint32_t pTick)
{
    mBody.clear();

    if (pTick != 0)
    {
        WireWriter::appendVarintField(mBody, WorldSnapshot::kTickFieldNumber, pTick);
    }
}

void SnapshotEncoder::addLeft(const std::vector<uint64_t>& pEntityIds)
{
    if (pEntityIds.empty())
    {
        return;
    }

    size_t tPayloadSize = 0;

    for (uint64_t tEntityId : pEntityIds)
    {
        tPayloadSize += WireWriter::getVarintSize(tEntityId);
    }

    WireWriter::appendLengthDelimitedHeader(mBody, WorldSnapshot::kLeftFieldNumber, tPayloadSize);

    for (uint64_t tEntityId : pEntityIds)
    {
        WireWriter::appendVarint(mBody, tEntityId);
    }
}

void SnapshotEncoder::setSelfCorrection(const services::world::EntityState& pState)
{
    appendMessage(WorldSnapshot::kSelfCorrectionFieldNumber, pState);
}

void SnapshotEncoder::setSelfVitals(const services::world::SelfVitals& pVitals)
{
    appendMessage(WorldSnapshot::kSelfVitalsFieldNumber, pVitals);
}

SnapshotEncoder::Packet SnapshotEncoder::finishMessage()
{
    constexpr size_t kLengthPrefixSize = sizeof(uint32_t);

    const size_t tBodySize = mBody.size();
    const size_t tResponseSize = WireWriter::getLengthDelimitedHeaderSize(protocol::Response::kWorldSnapshotFieldNumber, tBodySize) + tBodySize;
    const size_t tTotalSize = WireWriter::getLengthDelimitedHeaderSize(connection::Message::kResponseFieldNumber, tResponseSize) + tResponseSize;

    auto tPacket = std::make_shared<std::vector<char>>();
    tPacket->reserve(kLengthPrefixSize + tTotalSize);

    for (int tShift = 24; tShift >= 0; tShift -= 8)
    {
        tPacket->push_back(static_cast<char>((tTotalSize >> tShift) & 0xFF));
    }

    WireWriter::appendLengthDelimitedHeader(*tPacket, connection::Message::kResponseFieldNumber, tResponseSize);
    WireWriter::appendLengthDelimitedHeader(*tPacket, protocol::Response::kWorldSnapshotFieldNumber, tBodySize);
    WireWriter::appendBytes(*tPacket, mBody.data(), tBodySize);

    return tPacket;
}
