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

#if defined(LAKOT_RENDERER_OPENGL) || defined(LAKOT_RENDERER_OPENGLES)
    const AVertexArrayObject& getVertexArrayObject() const override;
#endif

protected:
#if defined(LAKOT_RENDERER_OPENGL) || defined(LAKOT_RENDERER_OPENGLES)
    VertexArrayObject mVertexArrayObject;
#endif
};

}

#endif
