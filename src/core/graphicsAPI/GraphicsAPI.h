#ifndef LAKOT_GRAPHICSAPI_H
#define LAKOT_GRAPHICSAPI_H

#include <iostream>
#include <sstream>
#include <string>

enum class GraphicsAPIType {
	eNONE,
	eOpenGL,
	eOpenGLES, /// TODO: will be implemented.
	eVulkan,   /// TODO: will be implemented.
	eD3Dx	   /// TODO: will be implemented.
};

using GraphicsAPIMajor = int;
using GraphicsAPIMinor = int;

class GraphicsAPI {
private:
	GraphicsAPIType mGraphicsAPIType;
	GraphicsAPIMajor mGraphicsAPIMajor;
	GraphicsAPIMinor mGraphicsAPIMinor;

	bool mIsGraphicsAPIInitialized;

public:
	GraphicsAPI(GraphicsAPIType tGraphicsAPIType, GraphicsAPIMajor tGraphicsAPIMajor, GraphicsAPIMinor tGraphicsAPIMinor);
	virtual ~GraphicsAPI();

	virtual void initializeGraphicsAPI() = 0;
	virtual void terminateGraphicsAPI() = 0;

	virtual std::string getGraphicsAPIString() = 0;

	GraphicsAPIMajor getGraphicsAPIMajor() const;
	GraphicsAPIMajor getGraphicsAPIMinor() const;

	void setIsGraphicsAPIInitialized(bool tIsGraphicsAPIInitialized);
	bool getIsGraphicsAPIInitialized() const;
};

#endif
