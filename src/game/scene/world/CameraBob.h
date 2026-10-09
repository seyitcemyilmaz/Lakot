#ifndef LAKOT_CAMERA_BOB_H
#define LAKOT_CAMERA_BOB_H

#include <glm/glm.hpp>

namespace lakot
{

class CameraBob
{
public:
    void update(float pDistance, double pDeltaTime);

    glm::vec3 getOffset(const glm::vec3& pRight, float pAlpha) const;

private:
    float mPhase{0.0f};
    float mWeight{0.0f};
    glm::vec2 mPrevious{0.0f};
    glm::vec2 mCurrent{0.0f};
};

}

#endif
