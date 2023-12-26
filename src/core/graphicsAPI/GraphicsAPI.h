#ifndef LAKOT_GRAPHICSAPI_H
#define LAKOT_GRAPHICSAPI_H

#include <sstream>
#include <string>

enum class GraphicsAPIType
{
    eNONE,
    eOpenGL,
    eOpenGLES, /// TODO: will be implemented.
    eVulkan,   /// TODO: will be implemented.
    eD3Dx	   /// TODO: will be implemented.
};

typedef int GraphicsAPIMajor;
typedef int GraphicsAPIMinor;

class GraphicsAPI
{
private:
    GraphicsAPIType mGraphicsAPIType;
    GraphicsAPIMajor mGraphicsAPIMajor;
    GraphicsAPIMinor mGraphicsAPIMinor;

    bool mIsGraphicsAPIInitialized;

public:
    virtual ~GraphicsAPI();
    GraphicsAPI(GraphicsAPIType tGraphicsAPIType, GraphicsAPIMajor tGraphicsAPIMajor, GraphicsAPIMinor tGraphicsAPIMinor);

    virtual void initializeGraphicsAPI() = 0;
    virtual void terminateGraphicsAPI() = 0;

    virtual std::string getGraphicsAPIString() = 0;

    GraphicsAPIMajor getGraphicsAPIMajor() const;
    GraphicsAPIMajor getGraphicsAPIMinor() const;

    void setIsGraphicsAPIInitialized(bool tIsGraphicsAPIInitialized);
    bool getIsGraphicsAPIInitialized() const;
};

#endif
