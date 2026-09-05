#ifndef LAKOT_BOX_CONTAINER_H
#define LAKOT_BOX_CONTAINER_H

#include <vector>

#include <glm/vec3.hpp>

#include "../render/Renderable.h"

namespace lakot
{

class BoxContainer final : public Renderable
{
public:
    ~BoxContainer() override;
    explicit BoxContainer();

    void initialize() override;
    void deinitialize() override;

    void addBox(const glm::vec3& pPosition, const glm::vec3& pSize, const glm::vec3& pColor = glm::vec3(1.0f, 0.5f, 0.2f));

    // Replaces every instance in one shot (vs. addBox's append) - used to
    // redraw a set of boxes whose membership/positions change over time.
    // pColors must be the same length as pPositions/pSizes.
    void setBoxes(const std::vector<glm::vec3>& pPositions, const std::vector<glm::vec3>& pSizes, const std::vector<glm::vec3>& pColors);

    unsigned int getInstanceCount();
};

}

#endif
