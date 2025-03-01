#ifndef LAKOT_OPENGLAPI_H
#define LAKOT_OPENGLAPI_H

#include "GraphicsAPI.h"

namespace lakot {

class OpenGLAPI final : public GraphicsAPI
{
public:
    virtual ~OpenGLAPI() override;
    OpenGLAPI();

    void initialize() override;
    void deinitialize() override;

    OpenGLType getOpenGLType() const;

private:
    OpenGLType mOpenGLType;
};

}

#endif
