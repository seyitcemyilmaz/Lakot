#include "graphicsapi.h"

GraphicsAPI::GraphicsAPI(GraphicsAPIType tGraphicsAPIType, GraphicsAPIMajor tGraphicsAPIMajor, GraphicsAPIMinor tGraphicsAPIMinor) :
    mGraphicsAPIType(tGraphicsAPIType), mGraphicsAPIMajor(tGraphicsAPIMajor), mGraphicsAPIMinor(tGraphicsAPIMinor) {
    mIsGraphicsAPIInitialized = false;
}

std::string GraphicsAPI::getGraphicsAPIString() {
    std::ostringstream tGraphicsAPIString;

    if (mGraphicsAPIType == GraphicsAPIType::eOpenGL) {
        tGraphicsAPIString << "OpenGL " << mGraphicsAPIMajor << "." << mGraphicsAPIMinor;
        return tGraphicsAPIString.str();
    } else if (mGraphicsAPIType == GraphicsAPIType::eOpenGLES) {
        throw "Not implemented.";
    } else if (mGraphicsAPIType == GraphicsAPIType::eVulkan) {
        throw "Not implemented.";
    } else if (mGraphicsAPIType == GraphicsAPIType::eD3Dx) {
        throw "Not implemented.";
    } else if (mGraphicsAPIType == GraphicsAPIType::eNONE) {
        throw "Undefined Graphics API Type.";
    }

    throw "Undefined behaviour.";
}

GraphicsAPIMajor GraphicsAPI::getGraphicsAPIMajor() {
    return mGraphicsAPIMajor;
}

GraphicsAPIMajor GraphicsAPI::getGraphicsAPIMinor() {
    return mGraphicsAPIMinor;
}

void GraphicsAPI::setIsGraphicsAPIInitialized(bool tIsGraphicsAPIInitialized) {
    mIsGraphicsAPIInitialized = tIsGraphicsAPIInitialized;
}

bool GraphicsAPI::getIsGraphicsAPIInitialized() {
    return mIsGraphicsAPIInitialized;
}

GraphicsAPI::~GraphicsAPI() {

}
