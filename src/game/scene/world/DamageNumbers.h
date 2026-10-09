#ifndef LAKOT_DAMAGE_NUMBERS_H
#define LAKOT_DAMAGE_NUMBERS_H

#include <cstdint>
#include <string>
#include <vector>

namespace lakot
{

struct DamageNumber
{
    uint64_t entityId = 0;
    std::string text;
    bool isAgainstLocal = false;
    bool isByLocal = false;
    float age = 0.0f;
};

class DamageNumbers
{
public:
    static constexpr float kLifetimeSeconds = 1.2f;
    static constexpr float kRiseUnits = 1.2f;

    void add(uint64_t pEntityId, uint32_t pDamage, bool pIsAgainstLocal, bool pIsByLocal);
    void update(float pDeltaSeconds);
    void clear();

    const std::vector<DamageNumber>& getAll() const;

private:
    std::vector<DamageNumber> mNumbers;
};

}

#endif
