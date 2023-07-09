#ifndef LAKOT_GRAPHICSAPI_H
#define LAKOT_GRAPHICSAPI_H

#include <iostream>
#include <sstream>
#include <string>

enum GraphicsAPIType {
	eNONE,
	eOpenGL,
	eOpenGLES,  /// TODO: will be implemented.
	eVulkan,    /// TODO: will be implemented.
	eD3Dx       /// TODO: will be implemented.
};

typedef int GraphicsAPIMajor;
typedef int GraphicsAPIMinor;

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

	GraphicsAPIMajor getGraphicsAPIMajor();
	GraphicsAPIMajor getGraphicsAPIMinor();

	void setIsGraphicsAPIInitialized(bool tIsGraphicsAPIInitialized);
	bool getIsGraphicsAPIInitialized();
};


#endif
