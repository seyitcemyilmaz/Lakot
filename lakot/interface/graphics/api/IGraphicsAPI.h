#ifndef LAKOT_IGRAPHICSAPI_H
#define LAKOT_IGRAPHICSAPI_H

#include "../../engine/IObject.h"
#include "../render/IRenderer.h"

namespace lakot {

enum class GraphicsAPIType
{
    eUndefined,
    eOpenGL,
    eOpenGLES
};

class IGraphicsAPI : public IObject
{
public:
    virtual ~IGraphicsAPI() = default;
    IGraphicsAPI() = default;

    virtual bool getIsInitialized() const = 0;

    virtual GraphicsAPIType getType() const = 0;

    virtual int getVersionMajor() const = 0;
    virtual int getVersionMinor() const = 0;

    virtual IRenderer* getRenderer() const = 0;
};

}

#endif
