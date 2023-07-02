#include "graphicsapi.h"

GraphicsAPI::GraphicsAPI(GraphicsAPIType tGraphicsAPIType, GraphicsAPIMajor tGraphicsAPIMajor, GraphicsAPIMinor tGraphicsAPIMinor) :
	mGraphicsAPIType(tGraphicsAPIType), mGraphicsAPIMajor(tGraphicsAPIMajor), mGraphicsAPIMinor(tGraphicsAPIMinor) {
	mIsGraphicsAPIInitialized = false;
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

GraphicsAPI::~GraphicsAPI() { }
