#ifndef LAKOT_EFFECT_DEFINITION_H
#define LAKOT_EFFECT_DEFINITION_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace lakot
{

enum class BlendType
{
    eAdditive,
    eAlpha
};

enum class ParticleOrientationType
{
    eBillboard,
    eVelocity,
    eGround
};

enum class EmitterShapeType
{
    ePoint,
    eSphere,
    eRing
};

struct FloatRange
{
    float min{0.0f};
    float max{0.0f};
};

template <typename ValueType>
struct CurveKey
{
    float time{0.0f};
    ValueType value{};
};

template <typename ValueType>
struct Curve
{
    std::array<CurveKey<ValueType>, 4> keys{};
    uint32_t count{0};

    ValueType evaluate(float pTime) const
    {
        if (count == 0)
        {
            return ValueType{};
        }

        if (pTime <= keys[0].time)
        {
            return keys[0].value;
        }

        for (uint32_t tIndex = 1; tIndex < count; tIndex++)
        {
            if (pTime <= keys[tIndex].time)
            {
                const CurveKey<ValueType>& tFrom = keys[tIndex - 1];
                const CurveKey<ValueType>& tTo = keys[tIndex];
                float tSpan = tTo.time - tFrom.time;
                float tFactor = tSpan > 0.0f ? (pTime - tFrom.time) / tSpan : 1.0f;
                return tFrom.value + (tTo.value - tFrom.value) * tFactor;
            }
        }

        return keys[count - 1].value;
    }
};

struct EmitterDefinition
{
    uint32_t textureIndex{0};
    BlendType blend{BlendType::eAdditive};
    ParticleOrientationType orientation{ParticleOrientationType::eBillboard};

    uint32_t burst{0};
    float rate{0.0f};
    float duration{0.0f};
    bool isLoop{false};

    FloatRange lifetime;
    FloatRange speed;
    glm::vec3 direction{0.0f, 1.0f, 0.0f};
    float spreadDegrees{0.0f};
    float gravity{0.0f};
    float drag{0.0f};
    float stretch{0.0f};

    EmitterShapeType shape{EmitterShapeType::ePoint};
    float shapeRadius{0.0f};
    glm::vec3 offset{0.0f};

    Curve<float> size;
    Curve<glm::vec4> color;

    FloatRange rotationDegrees;
    FloatRange spinDegrees;
};

struct EffectDefinition
{
    std::string name;
    std::vector<EmitterDefinition> emitters;
};

}

#endif
