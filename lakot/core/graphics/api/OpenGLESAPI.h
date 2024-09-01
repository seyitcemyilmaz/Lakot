#ifndef LAKOT_OPENGLESAPI_H
#define LAKOT_OPENGLESAPI_H

#include "GraphicsAPI.h"

namespace lakot {

class OpenGLESAPI final : public GraphicsAPI
{
public:
    virtual ~OpenGLESAPI() override;
    OpenGLESAPI();

    void initialize() override;
    void deinitialize() override;
};

}

#endif
