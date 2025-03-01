#ifndef LAKOT_ARENDERABLE_H
#define LAKOT_ARENDERABLE_H

#include <lakot/utilities/Object.h>
#include <lakot/utilities/Type.h>
#include <lakot/utilities/DataContainer.hpp>

namespace lakot {

class AVertexArrayObject;

class ARenderable : public Object
{
public:
    virtual ~ARenderable() override;
    ARenderable();

#if defined(LAKOT_RENDERER_OPENGL)
    virtual const AVertexArrayObject& getVertexArrayObject() const = 0;
#endif
    RenderableType getRenderableType() const;

    DataContainer& getVertexInformation();

protected:
    RenderableType mRenderableType;

    DataContainer mVertexInformation;
};

}

#endif
