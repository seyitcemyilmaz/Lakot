#include "Application.h"

#include <spdlog/spdlog.h>

#include "core/resource/ResourceManager.h"
#include "core/helper/FileManager.h"
#include "core/helper/shader/ShaderManager.h"
#include "core/helper/window/WindowManager.h"

#include "platform/PlatformFactory.h"

#include "application/graphics/model/ModelFactory.h"
#include "application/graphics/render/RenderManager.h"
#include "application/graphics/particle/ParticleManager.h"

#include "helper/controls/Keyboard.h"

Application::~Application()
{
    // ShaderManager::getInstance()->deleteShaders();

    delete mPlatform;
    delete mGUI;
}

Application::Application()
    : mPlatform(nullptr)
    , mGUI(nullptr)
    , mScene(nullptr)
    , mPostProcess(nullptr)
{

}

void Application::initialization()
{
    mPlatform = PlatformFactory::createPlatformObject();

    // TODO: will be changed !!!
    Window* tWindow = WindowManager::getInstance()->getWindow();
    mScene = new Scene();

    mScene->getProjection()->onResize(0,
                                      0,
                                      WindowManager::getInstance()->getWindowWidth(),
                                      WindowManager::getInstance()->getWindowHeight());

    mWindowSceneMap[tWindow] = {mScene};

    mGUI = new GUI();
    mGUI->initialize();

    initializeShaders();
    initializeModels();

    glEnable(GL_DEPTH_TEST);

    mPostProcess = new PostProcess();

    mPlatform->setRenderFunction([this] { render(); });
    mPlatform->setProcessInputFunction([this] { processInputs(); });

    WindowManager::getInstance()->getWindow()->setOnResize(
        [this](Window* pWindow, double pWidth, double pHeight)
        {
            onResize(pWindow, pWidth, pHeight);
        });
}

void Application::initializeShaders()
{
    ShaderManager::getInstance()->addShaderProgram(
        new ShaderProgram(
            "model",
            new Shader(FileManager::getInstance()->createPath(mPlatform->getAssetsPath(), "model.vsh"), ShaderType::eVertex),
            new Shader(FileManager::getInstance()->createPath(mPlatform->getAssetsPath(), "model.fsh"), ShaderType::eFragment)
        )
    );

    ShaderManager::getInstance()->addShaderProgram(
        new ShaderProgram(
            "particle",
            new Shader(FileManager::getInstance()->createPath(mPlatform->getAssetsPath(), "particle.vsh"), ShaderType::eVertex),
            new Shader(FileManager::getInstance()->createPath(mPlatform->getAssetsPath(), "particle.fsh"), ShaderType::eFragment)
        )
    );
}

void Application::initializeModels()
{
    ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "bmw\\scene.gltf"));
    Model* tModel0 = ModelFactory::createModel(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "bmw\\scene.gltf"));

    mScene->addModel(tModel0);
}

void Application::initializeParticles()
{
    ParticleManager::getInstance()->init();

    ParticleProvider* tParticleProvider = new ParticleProvider();
    ParticleManager::getInstance()->addParticleProvider(tParticleProvider);
}

void Application::onResize(Window* pWindow, double pWidth, double pHeight)
{
    const std::vector<Scene*> tScenes = mWindowSceneMap[pWindow];

    for (unsigned int i = 0; i < tScenes.size(); i++)
    {
        tScenes[i]->getProjection()->onResize(0, 0, pWidth, pHeight);
    }
}

void Application::processInputs()
{
    if (!WindowManager::getInstance()->getIsWindowFocused())
    {
        return;
    }


    if (!mGUI->isKeyboardEventOnGUI() && !mGUI->isMouseEventOnGUI())
    {
        if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_ESCAPE))
        {
            WindowManager::getInstance()->closeWindow();
        }

        mScene->processKeyboardInputs(mPlatform->getTimeDifference());
        mScene->processMouseInputs(mPlatform->getTimeDifference());
    }
}

void Application::render()
{
    mScene->update(mPlatform->getTimeDifference());
    RenderManager::getInstance()->renderScene(mScene);
    mGUI->render(mScene);
    mPostProcess->run();
}

void Application::run()
{
    mPlatform->run();
}

void Application::terminate()
{
    mPlatform->getGraphicsAPI()->terminateGraphicsAPI();
}
