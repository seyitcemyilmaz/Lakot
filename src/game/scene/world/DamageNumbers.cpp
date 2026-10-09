#include "DamageNumbers.h"

#include <algorithm>

using namespace lakot;

void DamageNumbers::add(uint64_t pEntityId, uint32_t pDamage, bool pIsAgainstLocal, bool pIsByLocal)
{
    mNumbers.push_back({ pEntityId, std::to_string(pDamage), pIsAgainstLocal, pIsByLocal, 0.0f });
}

void DamageNumbers::update(float pDeltaSeconds)
{
    for (DamageNumber& tNumber : mNumbers)
    {
        tNumber.age += pDeltaSeconds;
    }

    std::erase_if(mNumbers, [](const DamageNumber& pNumber) { return pNumber.age >= kLifetimeSeconds; });
}

void DamageNumbers::clear()
{
    mNumbers.clear();
}

const std::vector<DamageNumber>& DamageNumbers::getAll() const
{
    return mNumbers;
}
