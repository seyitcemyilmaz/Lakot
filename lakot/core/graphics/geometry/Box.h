#ifndef LAKOT_BOX_H
#define LAKOT_BOX_H

#include <lakot/abstract/render/ARenderable.h>

#include "../../render/VertexArrayObject.h"

namespace lakot {

class Box final : public ARenderable
{
public:
    virtual ~Box() override;
    Box();

    void initialize() override;
    void deinitialize() override;

    void addBox(const glm::vec3& pPosition, const glm::vec3& pSize);

#if defined(LAKOT_RENDERER_OPENGL)
    const AVertexArrayObject& getVertexArrayObject() const override;
#endif

protected:
    bool mIsInitialized;
    bool mIsNeedUpdate;

#if defined(LAKOT_RENDERER_OPENGL)
    VertexArrayObject mVertexArrayObject;
#endif
};

}

#endif
