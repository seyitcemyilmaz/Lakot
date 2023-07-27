#include "GraphicsAPI.h"

GraphicsAPI::GraphicsAPI(GraphicsAPIType tGraphicsAPIType, GraphicsAPIMajor tGraphicsAPIMajor, GraphicsAPIMinor tGraphicsAPIMinor) :
	mGraphicsAPIType(tGraphicsAPIType), mGraphicsAPIMajor(tGraphicsAPIMajor), mGraphicsAPIMinor(tGraphicsAPIMinor) {
	mIsGraphicsAPIInitialized = false;
}

GraphicsAPIMajor GraphicsAPI::getGraphicsAPIMajor() const {
	return mGraphicsAPIMajor;
}

GraphicsAPIMajor GraphicsAPI::getGraphicsAPIMinor() const {
	return mGraphicsAPIMinor;
}

void GraphicsAPI::setIsGraphicsAPIInitialized(bool tIsGraphicsAPIInitialized) {
	mIsGraphicsAPIInitialized = tIsGraphicsAPIInitialized;
}

bool GraphicsAPI::getIsGraphicsAPIInitialized() const {
	return mIsGraphicsAPIInitialized;
}

GraphicsAPI::~GraphicsAPI() = default;
