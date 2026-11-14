#ifndef LAKOT_CLIP_DEFINITION_H
#define LAKOT_CLIP_DEFINITION_H

#include <string>
#include <vector>

namespace lakot
{

enum class ClipEventType
{
    eHit,
    eEffect,
    eSound,
    eTrailOn,
    eTrailOff
};

struct ClipEvent
{
    float time{0.0f};
    ClipEventType type{ClipEventType::eEffect};
    std::string name;
    std::string socket;
};

struct ClipDefinition
{
    std::string source;
    float start{0.0f};
    float end{0.0f};
    float rate{1.0f};
    bool isLoop{true};
    bool isUpper{false};
    std::vector<ClipEvent> events;
};

}

#endif
