#include "WorldScene.h"

#include "../Engine.h"
#include "../gui/PanelLoader.h"
#include "../graphics/camera/CameraFactory.h"

using namespace lakot;

WorldScene::~WorldScene()
{

}

WorldScene::WorldScene(Engine& pEngine)
    : Scene(pEngine)
    , mBoxContainer(nullptr)
    , mCamera(nullptr)
{

}

void WorldScene::enter()
{
    SDL_Log("WorldScene: enter");

    GuiLayer& tGui = mEngine.getGuiLayer();
    tGui.clearPanels();
    PanelLoader::load(tGui, PanelContext::World);

    SDL_SetWindowRelativeMouseMode(mEngine.getWindow(), true);

    mCamera = std::make_unique<FPSCamera>();

    ShaderManager& tShaderManager = mEngine.getShaderManager();

    std::string tVertexSource = R"(
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aOffset;
        layout (location = 2) in vec3 aSize;

        uniform mat4 uViewProjection;

        void main()
        {
            vec3 tFinalPosition = (aPos * aSize) + aOffset;

            gl_Position = uViewProjection * vec4(tFinalPosition, 1.0);
        }
    )";

    std::string tFragmentSource = R"(
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(1.0, 0.5, 0.2, 1.0);
        }
    )";

    tShaderManager.createProgram("box_instanced_shader", tVertexSource, tFragmentSource);

    mBoxContainer = std::make_unique<BoxContainer>();
    mBoxContainer->initialize();

    mBoxContainer->addBox(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    mBoxContainer->addBox(glm::vec3(5.0f, 2.0f, -5.0f), glm::vec3(2.0f, 2.0f, 2.0f));
    mBoxContainer->addBox(glm::vec3(-5.0f, -2.0f, 0.0f), glm::vec3(1.0f, 3.0f, 1.0f));
}

void WorldScene::exit()
{
    SDL_Log("WorldScene: exit");

    SDL_SetWindowRelativeMouseMode(mEngine.getWindow(), false);
}

void WorldScene::update(double pDeltaTime)
{
    if (!mCamera)
    {
        return;
    }

    float tSpeed = 5.0f * static_cast<float>(pDeltaTime);
    glm::vec3 tMovement(0.0f);

    if (mIsMovingForward)  tMovement += mCamera->getFrontVector();
    if (mIsMovingBackward) tMovement -= mCamera->getFrontVector();
    if (mIsMovingRight)    tMovement += mCamera->getRightVector();
    if (mIsMovingLeft)     tMovement -= mCamera->getRightVector();

    if (glm::length(tMovement) > 0.0f)
    {
        tMovement = glm::normalize(tMovement) * tSpeed;
        mCamera->changePosition(tMovement);
        mCamera->update();
    }
}

void WorldScene::render()
{
    int tWidth;
    int tHeight;
    SDL_GetWindowSizeInPixels(mEngine.getWindow(), &tWidth, &tHeight);

    mCamera->updateProjection(tWidth, tHeight);
    mCamera->update();

    ShaderManager& tShaderManager = mEngine.getShaderManager();
    ShaderProgram* tBoxShader = tShaderManager.getProgram("box_instanced_shader");

    if (tBoxShader)
    {
        tBoxShader->bind();

        tBoxShader->setMat4("uViewProjection", mCamera->getViewProjectionMatrix());

        mBoxContainer->getVertexArrayObject().bind();

        glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr, mBoxContainer->getInstanceCount());

        tBoxShader->unbind();
    }
}

bool WorldScene::handleEvent(SDL_Event* pEvent)
{
    if (pEvent->type == SDL_EVENT_KEY_DOWN || pEvent->type == SDL_EVENT_KEY_UP)
    {
        bool tIsPressed = (pEvent->type == SDL_EVENT_KEY_DOWN);

        switch (pEvent->key.key)
        {
            case SDLK_W:
            {
                mIsMovingForward = tIsPressed;
                return true;
            }
            case SDLK_S:
            {
                mIsMovingBackward = tIsPressed;
                return true;
            }
            case SDLK_A:
            {
                mIsMovingLeft = tIsPressed;
                return true;
            }
            case SDLK_D:
            {
                mIsMovingRight = tIsPressed;
                return true;
            }
        }
    }

    if (pEvent->type == SDL_EVENT_MOUSE_MOTION)
    {
        float tXOffset = pEvent->motion.xrel;
        float tYOffset = -pEvent->motion.yrel;

        if (mCamera)
        {
            mCamera->processMouseMovement(tXOffset, tYOffset);
        }

        return true;
    }

    return false;
}
