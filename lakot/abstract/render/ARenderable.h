#ifndef LAKOT_ARENDERABLE_H
#define LAKOT_ARENDERABLE_H

#include <lakot/utilities/Object.h>
#include <lakot/utilities/Type.h>

namespace lakot {

class AVertexArrayObject;

class ARenderable : public Object
{
public:
    virtual ~ARenderable() override;
    ARenderable();

#if defined(LAKOT_RENDERER_OPENGL) || defined(LAKOT_RENDERER_OPENGLES)
    virtual const AVertexArrayObject& getVertexArrayObject() const = 0;
#endif
    RenderableType getRenderableType() const;

    VertexInformation& getVertexInformation();

protected:
    RenderableType mRenderableType;

    VertexInformation mVertexInformation;
};

}

#endif
