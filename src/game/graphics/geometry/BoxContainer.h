#ifndef LAKOT_BOX_CONTAINER_H
#define LAKOT_BOX_CONTAINER_H

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

    void addBox(const glm::vec3& pPosition, const glm::vec3& pSize);

    unsigned int getInstanceCount();
};

}

#endif
