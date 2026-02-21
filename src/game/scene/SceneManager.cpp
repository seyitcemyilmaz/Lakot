#include "SceneManager.h"

using namespace lakot;

SceneManager::SceneManager()
    : mCurrentScene(nullptr)
    , mNextScene(nullptr)
{

}

SceneManager::~SceneManager()
{
    if (mCurrentScene)
    {
        mCurrentScene->exit();
    }
}

void SceneManager::setNextScene(std::unique_ptr<Scene> pScene)
{
    mNextScene = std::move(pScene);
}

void SceneManager::update(double pDeltaTime)
{
    if (mNextScene)
    {
        applySceneChange();
    }

    if (mCurrentScene)
    {
        mCurrentScene->update(pDeltaTime);
    }
}

void SceneManager::render()
{
    if (mCurrentScene)
    {
        mCurrentScene->render();
    }
}

bool SceneManager::handleEvent(SDL_Event* pEvent)
{
    if (mCurrentScene)
    {
        return mCurrentScene->handleEvent(pEvent);
    }

    return false;
}

void SceneManager::applySceneChange()
{
    if (mCurrentScene)
    {
        mCurrentScene->exit();
    }

    mCurrentScene = std::move(mNextScene);

    if (mCurrentScene)
    {
        mCurrentScene->enter();
    }
}
