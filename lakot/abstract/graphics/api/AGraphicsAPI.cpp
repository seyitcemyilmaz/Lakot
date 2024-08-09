#include "AGraphicsAPI.h"

using namespace lakot;

AGraphicsAPI::~AGraphicsAPI() {}

AGraphicsAPI::AGraphicsAPI()
    : IGraphicsAPI()
    , mIsInitialized(false)
    , mType(GraphicsAPIType::eUndefined)
    , mVersionMajor(-1)
    , mVersionMinor(-1)
    , mRenderer(nullptr)
{

}

bool AGraphicsAPI::getIsInitialized() const
{
    return mIsInitialized;
}

GraphicsAPIType AGraphicsAPI::getType() const
{
    return mType;
}

int AGraphicsAPI::getVersionMajor() const
{
    return mVersionMajor;
}

int AGraphicsAPI::getVersionMinor() const
{
    return mVersionMinor;
}

IRenderer* AGraphicsAPI::getRenderer() const
{
    return mRenderer;
}
