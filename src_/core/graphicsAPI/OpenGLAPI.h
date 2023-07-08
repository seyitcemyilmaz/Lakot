#ifndef LAKOT_OPENGLAPI_H
#define LAKOT_OPENGLAPI_H

#include "GraphicsAPI.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class OpenGLAPI : public GraphicsAPI {
public:
	OpenGLAPI(GraphicsAPIMajor tGraphicsAPIMajor, GraphicsAPIMinor tGraphicsAPIMinor);
	~OpenGLAPI();

	void initializeGraphicsAPI() override;
	void terminateGraphicsAPI() override;

	std::string getGraphicsAPIString() override;
};

#endif
