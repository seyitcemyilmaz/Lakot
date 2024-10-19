#include "ALayer.h"

using namespace lakot;

ALayer::~ALayer()
{

}

ALayer::ALayer()
    : Object()
    , mScene(nullptr)
{

}

AScene* ALayer::getScene() const
{
    return mScene;
}
