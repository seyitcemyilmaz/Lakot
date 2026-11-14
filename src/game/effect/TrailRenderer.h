#ifndef LAKOT_TRAIL_RENDERER_H
#define LAKOT_TRAIL_RENDERER_H

#include <vector>

#include <glm/glm.hpp>

namespace lakot
{

class Camera;
class ShaderManager;
class WeaponTrail;

class TrailRenderer
{
public:
    explicit TrailRenderer(ShaderManager& pShaders);
    ~TrailRenderer();

    TrailRenderer(const TrailRenderer&) = delete;
    TrailRenderer& operator=(const TrailRenderer&) = delete;

    void render(const WeaponTrail& pTrail, const Camera& pCamera);

private:
    ShaderManager& mShaders;
    std::vector<glm::vec4> mVertices;
    std::vector<size_t> mRuns;
    unsigned int mVertexArray{0};
    unsigned int mBuffer{0};
    size_t mCapacity{0};
};

}

#endif
