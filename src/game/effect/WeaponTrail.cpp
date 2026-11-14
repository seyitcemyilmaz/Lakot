#include "WeaponTrail.h"

using namespace lakot;

namespace
{
    constexpr size_t kMaxSamples = 12;
    constexpr float kTeleportDistance = 4.0f;
}

void WeaponTrail::setActive(uint64_t pEntityId, bool pIsActive)
{
    if (pIsActive)
    {
        mTracks[pEntityId].isActive = true;
        return;
    }

    if (auto tFound = mTracks.find(pEntityId); tFound != mTracks.end())
    {
        tFound->second.isActive = false;
    }
}

void WeaponTrail::addSample(uint64_t pEntityId, const glm::vec3& pTip, const glm::vec3& pBase)
{
    std::deque<TrailSample>& tSamples = mTracks[pEntityId].samples;

    if (!tSamples.empty() && glm::distance(tSamples.back().tip, pTip) > kTeleportDistance)
    {
        tSamples.clear();
    }

    tSamples.push_back({ pTip, pBase, 0.0f });

    if (tSamples.size() > kMaxSamples)
    {
        tSamples.pop_front();
    }
}

void WeaponTrail::update(float pDeltaSeconds)
{
    for (auto tIterator = mTracks.begin(); tIterator != mTracks.end();)
    {
        std::deque<TrailSample>& tSamples = tIterator->second.samples;

        for (TrailSample& tSample : tSamples)
        {
            tSample.age += pDeltaSeconds;
        }

        while (!tSamples.empty() && tSamples.front().age > kLifetimeSeconds)
        {
            tSamples.pop_front();
        }

        if (tSamples.empty() && !tIterator->second.isActive)
        {
            tIterator = mTracks.erase(tIterator);
        }
        else
        {
            ++tIterator;
        }
    }
}

void WeaponTrail::retain(const std::function<bool(uint64_t)>& pIsKept)
{
    std::erase_if(mTracks, [&pIsKept](const auto& pEntry)
    {
        return !pIsKept(pEntry.first);
    });
}

void WeaponTrail::clear()
{
    mTracks.clear();
}

const WeaponTrail::Tracks& WeaponTrail::getTracks() const
{
    return mTracks;
}
