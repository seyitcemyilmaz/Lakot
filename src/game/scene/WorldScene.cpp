#include "WorldScene.h"

#include "../Engine.h"
#include "../gui/PanelLoader.h"

using namespace lakot;

WorldScene::~WorldScene()
{

}

WorldScene::WorldScene(Engine& pEngine)
    : Scene(pEngine)
    , mCamera(nullptr)
    , mBoxContainer(nullptr)
    , mTerrain(nullptr)
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

    std::string tBoxInstancedVertexSource = R"(
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

    std::string tBoxInstancedFragmentSource = R"(
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(1.0, 0.5, 0.2, 1.0);
        }
    )";

    std::string tTerrainVertexSource = R"(
        layout (location = 0) in vec3 aPos;

        uniform mat4 uViewProjection;

        out vec3 FragPos;

        void main()
        {
            FragPos = aPos;
            gl_Position = uViewProjection * vec4(aPos, 1.0);
        }
    )";

    // Fragment Shader
    std::string tTerrainFragmentSource = R"(
        in vec3 FragPos;
        out vec4 FragColor;

        float near = 0.1;
        float far  = 100.0;

        float LinearizeDepth(float depth)
        {
            float z = depth * 2.0 - 1.0;
            return (2.0 * near * far) / (far + near - z * (far - near));
        }

        void main() {
            vec3 nearColor = vec3(0.2, 0.6, 0.2);
            vec3 farColor = vec3(0.05, 0.15, 0.2);

            float depth = LinearizeDepth(gl_FragCoord.z);

            float factor = depth / far;

            // factor = pow(factor, 1.5);

            vec3 finalColor = mix(nearColor, farColor, factor);

            FragColor = vec4(finalColor, 1.0);
        }
    )";

    tShaderManager.createProgram("box_instanced", tBoxInstancedVertexSource, tBoxInstancedFragmentSource);
    tShaderManager.createProgram("terrain", tTerrainVertexSource, tTerrainFragmentSource);

    mBoxContainer = std::make_unique<BoxContainer>();
    mBoxContainer->initialize();

    mTerrain = std::make_unique<Terrain>(100, 100, 1.0f);
    mTerrain->initialize();

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

    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ShaderManager& tShaderManager = mEngine.getShaderManager();
    ShaderProgram* tBoxShader = tShaderManager.getProgram("box_instanced");

    if (tBoxShader)
    {
        tBoxShader->bind();

        tBoxShader->setMat4("uViewProjection", mCamera->getViewProjectionMatrix());

        mBoxContainer->getVertexArrayObject().bind();

        glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr, mBoxContainer->getInstanceCount());

        tBoxShader->unbind();
    }

    ShaderProgram* tTerrainShader = tShaderManager.getProgram("terrain");
    if (tTerrainShader && mTerrain)
    {
        tTerrainShader->bind();
        // tTerrainShader->setVec3("uCameraPos", mCamera->getPosition());
        tTerrainShader->setMat4("uViewProjection", mCamera->getViewProjectionMatrix());

        mTerrain->getVertexArrayObject().bind();
        glDrawElements(GL_TRIANGLES, mTerrain->getIndexCount(), GL_UNSIGNED_INT, 0);
        tTerrainShader->unbind();
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
