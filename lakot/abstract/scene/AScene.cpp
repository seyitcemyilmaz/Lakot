#include "AScene.h"

using namespace lakot;

AScene::~AScene()
{

}

AScene::AScene()
    : Object()
    , mActiveCamera(nullptr)
{

}

ACamera* AScene::getActiveCamera() const
{
    return mActiveCamera;
}
