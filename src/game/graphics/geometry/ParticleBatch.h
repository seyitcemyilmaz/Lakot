#ifndef LAKOT_PARTICLE_BATCH_H
#define LAKOT_PARTICLE_BATCH_H

#include <vector>

#include <glm/vec4.hpp>

#include "../render/Renderable.h"

namespace lakot
{

class ParticleBatch final : public Renderable
{
public:
    ~ParticleBatch() override;
    explicit ParticleBatch();

    void initialize() override;
    void deinitialize() override;

    void setInstances(const std::vector<glm::vec4>& pCenterSize, const std::vector<glm::vec4>& pColor,
                      const std::vector<glm::vec4>& pVelocityStretch, const std::vector<glm::vec4>& pParams);

    unsigned int getInstanceCount();
};

}

#endif
