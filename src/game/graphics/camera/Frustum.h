#ifndef LAKOT_FRUSTUM_H
#define LAKOT_FRUSTUM_H

#include <array>

#include <glm/glm.hpp>

namespace lakot
{

struct Frustum
{
    Frustum() = default;

    explicit Frustum(const glm::mat4& pViewProjection)
    {
        const glm::mat4 tRows = glm::transpose(pViewProjection);

        mPlanes[0] = tRows[3] + tRows[0];
        mPlanes[1] = tRows[3] - tRows[0];
        mPlanes[2] = tRows[3] + tRows[1];
        mPlanes[3] = tRows[3] - tRows[1];
        mPlanes[4] = tRows[3] + tRows[2];
        mPlanes[5] = tRows[3] - tRows[2];

        for (glm::vec4& tPlane : mPlanes)
        {
            tPlane /= glm::length(glm::vec3(tPlane));
        }
    }

    bool intersectsBox(const glm::vec3& pMin, const glm::vec3& pMax) const
    {
        for (const glm::vec4& tPlane : mPlanes)
        {
            const glm::vec3 tPositive(tPlane.x >= 0.0f ? pMax.x : pMin.x,
                                      tPlane.y >= 0.0f ? pMax.y : pMin.y,
                                      tPlane.z >= 0.0f ? pMax.z : pMin.z);

            if (glm::dot(glm::vec3(tPlane), tPositive) + tPlane.w < 0.0f)
            {
                return false;
            }
        }

        return true;
    }

    std::array<glm::vec4, 6> mPlanes{};
};

}

#endif
