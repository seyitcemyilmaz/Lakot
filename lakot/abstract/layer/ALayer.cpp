#include "ALayer.h"

using namespace lakot;

ALayer::~ALayer()
{

}

ALayer::ALayer()
    : ILayer()
    , mIsInitialized(false)
{

}

bool ALayer::getIsInitialized() const
{
    return mIsInitialized;
}
