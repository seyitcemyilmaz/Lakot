#ifndef LAKOT_OPENGLAPI_H
#define LAKOT_OPENGLAPI_H

#include "GraphicsAPI.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class OpenGLAPI : public GraphicsAPI
{
public:
    virtual ~OpenGLAPI();
    OpenGLAPI(GraphicsAPIMajor tGraphicsAPIMajor, GraphicsAPIMinor tGraphicsAPIMinor);

    void initializeGraphicsAPI() override;
    void terminateGraphicsAPI() override;

    std::string getGraphicsAPIString() override;
};

#endif
