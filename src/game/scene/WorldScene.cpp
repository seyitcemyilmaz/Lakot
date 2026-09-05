#include "WorldScene.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>

#include <glm/glm.hpp>

#include <imgui.h>

#include "../Engine.h"
#include "../gui/PanelLoader.h"
#include "../gui/UiTheme.h"

namespace
{
    // How often the local player's state is sent to the server, and how much
    // it must have changed since the last send to be worth sending again.
    constexpr double kStateSendInterval = 0.1;
    constexpr float kMinPositionDeltaSq = 0.01f;
    constexpr float kMinYawDelta = 0.01f;

    // How long a remote player's box takes to glide from its previous known
    // position to the newly received one, instead of snapping to it.
    constexpr double kInterpolationDuration = kStateSendInterval;

    // Temporary, placeholder-box-era color coding so players are visually
    // distinguishable from each other and from static decor - to be
    // replaced once real character appearance exists. The local player
    // always renders in this fixed color; every other player gets a color
    // deterministically derived from their id, so it stays consistent
    // across updates without any extra network state.
    const glm::vec3 kLocalCharacterColor(0.2f, 0.9f, 1.0f);

    glm::vec3 hsvToRgb(float pHue, float pSaturation, float pValue)
    {
        float tC = pValue * pSaturation;
        float tHPrime = pHue * 6.0f;
        float tX = tC * (1.0f - std::fabs(std::fmod(tHPrime, 2.0f) - 1.0f));
        float tM = pValue - tC;

        glm::vec3 tRgb;

        if (tHPrime < 1.0f)      tRgb = glm::vec3(tC, tX, 0.0f);
        else if (tHPrime < 2.0f) tRgb = glm::vec3(tX, tC, 0.0f);
        else if (tHPrime < 3.0f) tRgb = glm::vec3(0.0f, tC, tX);
        else if (tHPrime < 4.0f) tRgb = glm::vec3(0.0f, tX, tC);
        else if (tHPrime < 5.0f) tRgb = glm::vec3(tX, 0.0f, tC);
        else                     tRgb = glm::vec3(tC, 0.0f, tX);

        return tRgb + glm::vec3(tM);
    }

    glm::vec3 getColorForPlayerId(uint64_t pPlayerId)
    {
        uint64_t tHash = std::hash<uint64_t>{}(pPlayerId);
        float tHue = static_cast<float>(tHash % 360u) / 360.0f;
        return hsvToRgb(tHue, 0.65f, 0.95f);
    }

    // Nameplate offset is applied in SCREEN-SPACE pixels, after projection,
    // rather than raising the point in world space before projecting.
    // Raising it in world space made the apparent gap balloon with
    // perspective (further from the character box the closer the camera
    // got, or at certain pitches) - projecting the character's own position
    // and nudging the text up by a constant pixel amount keeps the nameplate
    // hugging the character's head consistently at any camera distance.
    constexpr float kNameplatePixelOffset = 55.0f;

    // Below this much accumulated mouse motion (pixels), a right-click
    // release is treated as a click (pick/menu) rather than a camera-orbit
    // drag.
    constexpr float kClickDragThresholdPixels = 6.0f;

    // Half-extents of the (1,2,1) placeholder box used for both remote
    // players and the local character everywhere else in this file -
    // picking tests a ray against this same box so the clickable area
    // matches what's actually drawn.
    constexpr glm::vec3 kPlayerBoxHalfExtents(0.5f, 1.0f, 0.5f);
}

using namespace lakot;

WorldScene::~WorldScene()
{

}

WorldScene::WorldScene(Engine& pEngine, const std::optional<InitialSpawnState>& pSpawnState, const std::string& pUsername)
    : Scene(pEngine)
    , mCamera(nullptr)
    , mBoxContainer(nullptr)
    , mTerrain(nullptr)
    , mInitialSpawnState(pSpawnState)
    , mLocalUsername(pUsername)
{

}

void WorldScene::enter()
{
    SDL_Log("WorldScene: enter");

    GuiLayer& tGui = mEngine.getGuiLayer();
    tGui.clearPanels();
    PanelLoader::load(tGui, PanelContext::World);

    // SDL_SetWindowRelativeMouseMode(mEngine.getWindow(), true);

    mCamera = std::make_unique<ThirdPersonCamera>();

    if (mInitialSpawnState)
    {
        mCamera->setTargetPosition(glm::vec3(mInitialSpawnState->x, mInitialSpawnState->y, mInitialSpawnState->z));
        mCamera->setYaw(glm::degrees(mInitialSpawnState->yaw));
        mCurrentMapId = mInitialSpawnState->mapId;
    }

    ShaderManager& tShaderManager = mEngine.getShaderManager();

    std::string tBoxInstancedVertexSource = R"(
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aOffset;
        layout (location = 2) in vec3 aSize;
        layout (location = 3) in vec3 aColor;

        uniform mat4 uViewProjection;

        out vec3 vColor;

        void main()
        {
            vec3 tFinalPosition = (aPos * aSize) + aOffset;

            gl_Position = uViewProjection * vec4(tFinalPosition, 1.0);
            vColor = aColor;
        }
    )";

    std::string tBoxInstancedFragmentSource = R"(
        in vec3 vColor;
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(vColor, 1.0);
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

        uniform vec3 uTint;

        void main()
        {
            if (FragPos.y < 0)
            {
                discard;
            }

            vec3 nearColor = vec3(0.2, 0.6, 0.2);
            vec3 farColor = vec3(0.05, 0.15, 0.2);

            float depth = LinearizeDepth(gl_FragCoord.z);

            float factor = depth / far;

            // factor = pow(factor, 1.5);

            vec3 finalColor = mix(nearColor, farColor, factor) * uTint;

            FragColor = vec4(finalColor, 1.0);
        }
    )";

    tShaderManager.createProgram("box_instanced", tBoxInstancedVertexSource, tBoxInstancedFragmentSource);
    tShaderManager.createProgram("terrain", tTerrainVertexSource, tTerrainFragmentSource);

    mBoxContainer = std::make_unique<BoxContainer>();
    mBoxContainer->initialize();

    mTerrain = std::make_unique<Terrain>(100, 100, 1.0f);
    mTerrain->initialize();

    applyMapVisuals();

    mRemotePlayerBoxes = std::make_unique<BoxContainer>();
    mRemotePlayerBoxes->initialize();

    mLocalCharacterBox = std::make_unique<BoxContainer>();
    mLocalCharacterBox->initialize();

    WorldController& tWorldController = mEngine.getNetworkManager().getWorldController();

    tWorldController.setPlayerJoinedCallback(
    [this](const WorldController::PlayerSnapshot& pSnapshot)
    {
        this->onPlayerJoined(pSnapshot);
    });

    tWorldController.setPlayerStateUpdateCallback(
    [this](const WorldController::PlayerSnapshot& pSnapshot)
    {
        this->onPlayerStateUpdate(pSnapshot);
    });

    tWorldController.setPlayerLeftCallback(
    [this](uint64_t pPlayerId)
    {
        this->onPlayerLeft(pPlayerId);
    });

    tWorldController.setMapChangedCallback(
    [this](const WorldController::MapSnapshot& pSnapshot)
    {
        this->onMapChanged(pSnapshot);
    });

    ChatController& tChatController = mEngine.getNetworkManager().getChatController();

    tChatController.setDirectMessageReceivedCallback(
    [this](uint64_t pFromPlayerId, const std::string& pFromUsername, const std::string& pText)
    {
        this->onDirectMessageReceived(pFromPlayerId, pFromUsername, pText);
    });

    tChatController.setDirectMessageResultCallback(
    [](bool pIsSuccess, const std::string& pMessage)
    {
        if (!pIsSuccess)
        {
            SDL_Log("Chat: %s", pMessage.c_str());
        }
    });
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

    // Flattened to the ground plane - classic MMO characters don't
    // climb/descend just because the camera is pitched up or down.
    glm::vec3 tFront = mCamera->getFrontVector();
    tFront.y = 0.0f;
    if (glm::length(tFront) > 0.0001f) tFront = glm::normalize(tFront);

    glm::vec3 tRight = mCamera->getRightVector();
    tRight.y = 0.0f;
    if (glm::length(tRight) > 0.0001f) tRight = glm::normalize(tRight);

    if (mIsMovingForward)  tMovement += tFront;
    if (mIsMovingBackward) tMovement -= tFront;
    if (mIsMovingRight)    tMovement += tRight;
    if (mIsMovingLeft)     tMovement -= tRight;

    if (glm::length(tMovement) > 0.0f)
    {
        tMovement = glm::normalize(tMovement) * tSpeed;
        mCamera->moveTarget(tMovement);
    }

    mStateSendAccumulator += pDeltaTime;

    if (mStateSendAccumulator >= kStateSendInterval)
    {
        mStateSendAccumulator = 0.0;
        sendLocalPlayerState();
    }

    for (auto& [tPlayerId, tVisual] : mRemotePlayers)
    {
        tVisual.interpolationElapsed += pDeltaTime;
    }

    rebuildRemotePlayerBoxes();

    if (mLocalCharacterBox)
    {
        std::vector<glm::vec3> tPositions{ mCamera->getTargetPosition() };
        std::vector<glm::vec3> tSizes{ glm::vec3(1.0f, 2.0f, 1.0f) };
        std::vector<glm::vec3> tColors{ kLocalCharacterColor };
        mLocalCharacterBox->setBoxes(tPositions, tSizes, tColors);
    }
}

void WorldScene::render()
{
    int tWidth;
    int tHeight;
    SDL_GetWindowSizeInPixels(mEngine.getWindow(), &tWidth, &tHeight);
    mViewportWidth = tWidth;
    mViewportHeight = tHeight;

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

        if (mRemotePlayerBoxes && mRemotePlayerBoxes->getInstanceCount() > 0)
        {
            mRemotePlayerBoxes->getVertexArrayObject().bind();
            glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr, mRemotePlayerBoxes->getInstanceCount());
        }

        if (mLocalCharacterBox && mLocalCharacterBox->getInstanceCount() > 0)
        {
            mLocalCharacterBox->getVertexArrayObject().bind();
            glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr, mLocalCharacterBox->getInstanceCount());
        }

        tBoxShader->unbind();
    }

    ShaderProgram* tTerrainShader = tShaderManager.getProgram("terrain");
    if (tTerrainShader && mTerrain)
    {
        tTerrainShader->bind();
        // tTerrainShader->setVec3("uCameraPos", mCamera->getPosition());
        tTerrainShader->setMat4("uViewProjection", mCamera->getViewProjectionMatrix());

        // Cheap per-map visual distinction without a real content pipeline -
        // Town renders at natural color, Forest gets a strong warm/autumn
        // shift (pushes the green/blue ground gradient toward orange-brown)
        // so the two are unmistakable at a glance, not just subtly different.
        glm::vec3 tTint = (mCurrentMapId == 0) ? glm::vec3(1.0f, 1.0f, 1.0f) : glm::vec3(1.6f, 0.55f, 0.3f);
        tTerrainShader->setVec3("uTint", tTint);

        mTerrain->getVertexArrayObject().bind();
        glDrawElements(GL_TRIANGLES, mTerrain->getIndexCount(), GL_UNSIGNED_INT, 0);
        tTerrainShader->unbind();
    }

    // Nameplates - drawn last, as a screen-space overlay on top of the 3D
    // scene (ImGui's own render happens later in GuiLayer::end(), so
    // anything queued into its draw list here still shows up this frame).
    renderNameplate(mCamera->getTargetPosition(), mLocalUsername);

    for (const auto& [tPlayerId, tVisual] : mRemotePlayers)
    {
        renderNameplate(getInterpolatedPosition(tVisual), tVisual.username);
    }

    // Right-click target context menu - a plain ImGui popup rather than a
    // Panel, since it's transient/positional (opens at the cursor, closes
    // on any outside click) rather than a persistent window. Extending to
    // monster targets later just means branching on target.kind here.
    if (mContextMenuOpenRequested)
    {
        ImGui::SetNextWindowPos(toGlobalImGuiPos(ImVec2(mContextMenuScreenPos.x, mContextMenuScreenPos.y)));
        ImGui::OpenPopup("TargetContextMenu");
        mContextMenuOpenRequested = false;
    }

    if (ImGui::BeginPopup("TargetContextMenu"))
    {
        if (mContextMenuTarget)
        {
            ImGui::TextDisabled("%s", mContextMenuTarget->name.c_str());
            ImGui::Separator();

            if (ImGui::MenuItem("Chat"))
            {
                openChatWith(mContextMenuTarget->id, mContextMenuTarget->name);
                ImGui::CloseCurrentPopup();
            }

            ImGui::BeginDisabled();
            ImGui::MenuItem("Trade"); // placeholder - wired up later
            ImGui::EndDisabled();
        }

        ImGui::EndPopup();
    }
    else
    {
        // Popup isn't open anymore (dismissed, or just closed above) -
        // release the target it referred to.
        mContextMenuTarget.reset();
    }

    renderUnreadChatIndicators();
}

bool WorldScene::handleEvent(SDL_Event* pEvent)
{
    if (pEvent->type == SDL_EVENT_KEY_DOWN || pEvent->type == SDL_EVENT_KEY_UP)
    {
        bool tIsPressed = (pEvent->type == SDL_EVENT_KEY_DOWN);

        // While an ImGui text field (the chat window's input box) has
        // keyboard focus, WASD should type into it instead of moving the
        // character. A key-UP still goes through even then, so releasing a
        // key right after clicking into chat doesn't leave that direction
        // stuck "held".
        if (tIsPressed && ImGui::GetIO().WantCaptureKeyboard)
        {
            return false;
        }

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

    if (pEvent->type == SDL_EVENT_MOUSE_BUTTON_DOWN && pEvent->button.button == SDL_BUTTON_RIGHT)
    {
        // Let a hovered ImGui window (chat, context menu) have the click -
        // don't also start orbiting or treat it as a world target-pick.
        if (ImGui::GetIO().WantCaptureMouse)
        {
            return false;
        }

        mIsOrbiting = true;
        mOrbitDragDistance = 0.0f;
        mRightClickDownPos = glm::vec2(pEvent->button.x, pEvent->button.y);
        SDL_SetWindowRelativeMouseMode(mEngine.getWindow(), true);
        return true;
    }

    if (pEvent->type == SDL_EVENT_MOUSE_BUTTON_UP && pEvent->button.button == SDL_BUTTON_RIGHT)
    {
        bool tWasOrbiting = mIsOrbiting;
        mIsOrbiting = false;
        SDL_SetWindowRelativeMouseMode(mEngine.getWindow(), false);

        // A right-click that barely moved is a target-pick, not a camera
        // orbit - open the context menu next frame's render() (ImGui popups
        // can only be opened while the ImGui frame is active).
        if (tWasOrbiting && mOrbitDragDistance < kClickDragThresholdPixels)
        {
            if (auto tTarget = pickTargetAt(mRightClickDownPos))
            {
                mContextMenuTarget = *tTarget;
                mContextMenuScreenPos = mRightClickDownPos;
                mContextMenuOpenRequested = true;
            }
        }

        return true;
    }

    if (pEvent->type == SDL_EVENT_MOUSE_MOTION)
    {
        if (mIsOrbiting && mCamera)
        {
            float tXOffset = pEvent->motion.xrel;
            float tYOffset = -pEvent->motion.yrel;

            mOrbitDragDistance += std::fabs(pEvent->motion.xrel) + std::fabs(pEvent->motion.yrel);

            mCamera->orbit(tXOffset, tYOffset);
        }

        return true;
    }

    if (pEvent->type == SDL_EVENT_MOUSE_WHEEL)
    {
        // Let scrolling a chat window's history (or any other ImGui panel)
        // scroll it instead of also zooming the world camera underneath.
        if (ImGui::GetIO().WantCaptureMouse)
        {
            return false;
        }

        if (mCamera)
        {
            mCamera->zoom(pEvent->wheel.y);
        }

        return true;
    }

    return false;
}

void WorldScene::onPlayerJoined(const WorldController::PlayerSnapshot& pSnapshot)
{
    glm::vec3 tPosition(pSnapshot.x, pSnapshot.y, pSnapshot.z);

    // Appears immediately at the right spot - nothing to glide from yet.
    RemotePlayerVisual tVisual;
    tVisual.previousPosition = tPosition;
    tVisual.targetPosition = tPosition;
    tVisual.yaw = pSnapshot.yaw;
    tVisual.interpolationElapsed = kInterpolationDuration;
    tVisual.username = pSnapshot.username;

    mRemotePlayers[pSnapshot.playerId] = tVisual;
}

void WorldScene::onPlayerStateUpdate(const WorldController::PlayerSnapshot& pSnapshot)
{
    auto tIterator = mRemotePlayers.find(pSnapshot.playerId);

    if (tIterator == mRemotePlayers.end())
    {
        return; // no join seen for this id yet - ignore a stray update
    }

    RemotePlayerVisual& tVisual = tIterator->second;

    // Start the next glide from wherever the box currently is on screen
    // (not the old target) so back-to-back updates don't cause a jump.
    tVisual.previousPosition = getInterpolatedPosition(tVisual);
    tVisual.targetPosition = glm::vec3(pSnapshot.x, pSnapshot.y, pSnapshot.z);
    tVisual.yaw = pSnapshot.yaw;
    tVisual.interpolationElapsed = 0.0;
}

void WorldScene::onPlayerLeft(uint64_t pPlayerId)
{
    mRemotePlayers.erase(pPlayerId);
}

void WorldScene::onMapChanged(const WorldController::MapSnapshot& pSnapshot)
{
    // A portal fired server-side - none of the old map's players belong in
    // this view any more, and the server has already fully committed the
    // move, so just reflect what it told us.
    mRemotePlayers.clear();

    if (mCamera)
    {
        mCamera->setTargetPosition(glm::vec3(pSnapshot.x, pSnapshot.y, pSnapshot.z));
        mCamera->setYaw(glm::degrees(pSnapshot.yaw));
    }

    mCurrentMapId = pSnapshot.mapId;
    mHasSentInitialState = false;

    applyMapVisuals();
}

glm::vec3 WorldScene::getInterpolatedPosition(const RemotePlayerVisual& pVisual) const
{
    float tProgress = static_cast<float>(std::min(pVisual.interpolationElapsed / kInterpolationDuration, 1.0));
    return glm::mix(pVisual.previousPosition, pVisual.targetPosition, tProgress);
}

void WorldScene::rebuildRemotePlayerBoxes()
{
    if (!mRemotePlayerBoxes)
    {
        return;
    }

    std::vector<glm::vec3> tPositions;
    std::vector<glm::vec3> tSizes;
    std::vector<glm::vec3> tColors;

    tPositions.reserve(mRemotePlayers.size());
    tSizes.reserve(mRemotePlayers.size());
    tColors.reserve(mRemotePlayers.size());

    for (const auto& [tPlayerId, tVisual] : mRemotePlayers)
    {
        tPositions.push_back(getInterpolatedPosition(tVisual));
        tSizes.emplace_back(1.0f, 2.0f, 1.0f);
        tColors.push_back(getColorForPlayerId(tPlayerId));
    }

    mRemotePlayerBoxes->setBoxes(tPositions, tSizes, tColors);
}

void WorldScene::applyMapVisuals()
{
    if (!mBoxContainer)
    {
        return;
    }

    const glm::vec3 tTownDecorColor(1.0f, 0.5f, 0.2f);
    const glm::vec3 tForestDecorColor(0.35f, 0.22f, 0.1f);
    const glm::vec3 tPortalColor(1.0f, 0.0f, 0.0f);

    std::vector<glm::vec3> tPositions;
    std::vector<glm::vec3> tSizes;
    std::vector<glm::vec3> tColors;

    if (mCurrentMapId == 0) // Town
    {
        tPositions.emplace_back(0.0f, 0.0f, 0.0f);
        tSizes.emplace_back(1.0f, 1.0f, 1.0f);
        tColors.push_back(tTownDecorColor);

        tPositions.emplace_back(5.0f, 2.0f, -5.0f);
        tSizes.emplace_back(2.0f, 2.0f, 2.0f);
        tColors.push_back(tTownDecorColor);

        tPositions.emplace_back(-5.0f, -2.0f, 0.0f);
        tSizes.emplace_back(1.0f, 3.0f, 1.0f);
        tColors.push_back(tTownDecorColor);
    }
    else // Forest
    {
        tPositions.emplace_back(-6.0f, 1.0f, 3.0f);
        tSizes.emplace_back(1.0f, 4.0f, 1.0f);
        tColors.push_back(tForestDecorColor);

        tPositions.emplace_back(-3.0f, 0.5f, -4.0f);
        tSizes.emplace_back(1.5f, 1.0f, 1.5f);
        tColors.push_back(tForestDecorColor);

        tPositions.emplace_back(4.0f, 1.5f, 6.0f);
        tSizes.emplace_back(1.0f, 3.0f, 1.0f);
        tColors.push_back(tForestDecorColor);
    }

    // Portal marker - both maps' outgoing portal sits at the same local
    // coordinate (matches MapCatalog on the server), sized to stand out and
    // colored red to be easy to spot and walk into.
    tPositions.emplace_back(15.0f, 1.0f, 0.0f);
    tSizes.emplace_back(2.0f, 4.0f, 2.0f);
    tColors.push_back(tPortalColor);

    mBoxContainer->setBoxes(tPositions, tSizes, tColors);
}

void WorldScene::sendLocalPlayerState()
{
    if (!mCamera)
    {
        return;
    }

    const glm::vec3& tPosition = mCamera->getTargetPosition();
    const glm::vec3& tFront = mCamera->getFrontVector();
    float tYaw = std::atan2(tFront.z, tFront.x);

    if (mHasSentInitialState)
    {
        glm::vec3 tDelta = tPosition - mLastSentPosition;
        float tPositionDeltaSq = glm::dot(tDelta, tDelta);
        float tYawDelta = std::fabs(tYaw - mLastSentYaw);

        if (tPositionDeltaSq < kMinPositionDeltaSq && tYawDelta < kMinYawDelta)
        {
            return;
        }
    }

    mEngine.getNetworkManager().getWorldController().sendPlayerStateUpdate(tPosition.x, tPosition.y, tPosition.z, tYaw);

    mLastSentPosition = tPosition;
    mLastSentYaw = tYaw;
    mHasSentInitialState = true;
}

void WorldScene::renderNameplate(const glm::vec3& pWorldPosition, const std::string& pName)
{
    if (pName.empty() || !mCamera)
    {
        return;
    }

    // Project the character's own position (not a world-space-raised head
    // point) - raising the point in world space before projecting makes the
    // apparent screen-space offset balloon with perspective/distance, which
    // is what pushed the text far above the character. Instead project the
    // base position and add a small, constant offset in screen pixels.
    glm::vec4 tClip = mCamera->getViewProjectionMatrix() * glm::vec4(pWorldPosition, 1.0f);

    if (tClip.w <= 0.01f)
    {
        return; // behind (or right at) the camera
    }

    glm::vec3 tNdc = glm::vec3(tClip) / tClip.w;

    if (tNdc.x < -1.2f || tNdc.x > 1.2f || tNdc.y < -1.2f || tNdc.y > 1.2f)
    {
        return; // well outside the viewport
    }

    // Map NDC into the SAME physical-pixel space mCamera's projection aspect
    // ratio was built from (mViewportWidth/Height, refreshed in render()),
    // then convert that into ImGui's own coordinate space via
    // DisplayFramebufferScale. Using ImGui::GetIO().DisplaySize directly
    // here would silently disagree with the 3D projection whenever the
    // window's per-monitor DPI scale isn't 1 - on screens where it is, the
    // two happen to match and nothing looks wrong, which is why this only
    // showed up on some windows/monitors and not others.
    ImGuiIO& tIo = ImGui::GetIO();
    ImVec2 tFramebufferScale = tIo.DisplayFramebufferScale;
    if (tFramebufferScale.x <= 0.0f) tFramebufferScale.x = 1.0f;
    if (tFramebufferScale.y <= 0.0f) tFramebufferScale.y = 1.0f;

    float tPhysicalX = (tNdc.x * 0.5f + 0.5f) * static_cast<float>(mViewportWidth);
    float tPhysicalY = (1.0f - (tNdc.y * 0.5f + 0.5f)) * static_cast<float>(mViewportHeight);

    ImVec2 tScreenPos(
        tPhysicalX / tFramebufferScale.x,
        tPhysicalY / tFramebufferScale.y
    );

    ImVec2 tTextSize = ImGui::CalcTextSize(pName.c_str());
    ImVec2 tCentered(tScreenPos.x - tTextSize.x * 0.5f, tScreenPos.y - kNameplatePixelOffset - tTextSize.y);

    // GuiLayer enables ImGuiConfigFlags_ViewportsEnable, which puts every
    // ImGui draw-list coordinate (including GetBackgroundDrawList()'s) in
    // GLOBAL desktop space rather than window-local space - see
    // toGlobalImGuiPos() for why.
    tCentered = toGlobalImGuiPos(tCentered);

    // The BACKGROUND draw list (not foreground) - it's composited before any
    // ImGui window, so windows (the chat panel, the context menu, ...) draw
    // over it instead of the nameplate text sitting on top of them. It still
    // renders over the raw OpenGL 3D scene, since that's drawn earlier in
    // the frame, outside ImGui entirely - exactly the layering we want:
    // world < nameplates < UI windows.
    ImDrawList* tDrawList = ImGui::GetBackgroundDrawList(ImGui::GetMainViewport());

    // A small drop shadow keeps the name legible over any background.
    tDrawList->AddText(ImVec2(tCentered.x + 1.0f, tCentered.y + 1.0f), IM_COL32(0, 0, 0, 200), pName.c_str());
    tDrawList->AddText(tCentered, IM_COL32(255, 255, 255, 255), pName.c_str());
}

ImVec2 WorldScene::toGlobalImGuiPos(const ImVec2& pWindowLocalPos) const
{
    // GuiLayer enables ImGuiConfigFlags_ViewportsEnable, under which every
    // ImGui coordinate (draw-list positions, SetNextWindowPos, ...) is in
    // GLOBAL desktop space rather than this window's own local space - the
    // main viewport's Pos is this window's position on the desktop, (0,0)
    // only when the window happens to sit at the screen's origin. Skipping
    // this made screen-space UI (nameplates, and now the context menu) sit
    // at a fixed spot on the monitor instead of tracking the window.
    ImGuiViewport* tViewport = ImGui::GetMainViewport();
    ImVec2 tOffset = tViewport ? tViewport->Pos : ImVec2(0.0f, 0.0f);

    return ImVec2(pWindowLocalPos.x + tOffset.x, pWindowLocalPos.y + tOffset.y);
}

std::optional<WorldScene::TargetInfo> WorldScene::pickTargetAt(const glm::vec2& pWindowLocalPixel) const
{
    if (!mCamera)
    {
        return std::nullopt;
    }

    // The mouse coordinates SDL hands us are in the window's logical size
    // (SDL_GetWindowSize), not its physical framebuffer size
    // (SDL_GetWindowSizeInPixels, what mViewportWidth/Height track) - the
    // window was created with SDL_WINDOW_HIGH_PIXEL_DENSITY, so under DPI
    // scaling those two differ. Normalizing by the logical size here (to
    // get a 0..1 fraction of the window) sidesteps that entirely: the
    // fraction is the same regardless of which size is used, as long as
    // the numerator and denominator are in the same space.
    int tLogicalWidth = 1;
    int tLogicalHeight = 1;
    SDL_GetWindowSize(mEngine.getWindow(), &tLogicalWidth, &tLogicalHeight);

    if (tLogicalWidth <= 0 || tLogicalHeight <= 0)
    {
        return std::nullopt;
    }

    float tNdcX = (pWindowLocalPixel.x / static_cast<float>(tLogicalWidth)) * 2.0f - 1.0f;
    float tNdcY = 1.0f - (pWindowLocalPixel.y / static_cast<float>(tLogicalHeight)) * 2.0f;

    glm::mat4 tInverseViewProjection = glm::inverse(mCamera->getViewProjectionMatrix());

    glm::vec4 tNearPoint = tInverseViewProjection * glm::vec4(tNdcX, tNdcY, -1.0f, 1.0f);
    glm::vec4 tFarPoint = tInverseViewProjection * glm::vec4(tNdcX, tNdcY, 1.0f, 1.0f);

    if (std::fabs(tNearPoint.w) < 1e-6f || std::fabs(tFarPoint.w) < 1e-6f)
    {
        return std::nullopt;
    }

    glm::vec3 tRayOrigin = glm::vec3(tNearPoint) / tNearPoint.w;
    glm::vec3 tRayEnd = glm::vec3(tFarPoint) / tFarPoint.w;
    glm::vec3 tRayDirection = tRayEnd - tRayOrigin;

    if (glm::length(tRayDirection) < 1e-8f)
    {
        return std::nullopt;
    }

    tRayDirection = glm::normalize(tRayDirection);

    bool tHasHit = false;
    float tClosestDistance = std::numeric_limits<float>::max();
    uint64_t tClosestPlayerId = 0;

    for (const auto& [tPlayerId, tVisual] : mRemotePlayers)
    {
        glm::vec3 tCenter = getInterpolatedPosition(tVisual);
        glm::vec3 tBoxMin = tCenter - kPlayerBoxHalfExtents;
        glm::vec3 tBoxMax = tCenter + kPlayerBoxHalfExtents;

        // Standard slab-based ray-vs-AABB test.
        float tEntry = 0.0f;
        float tExit = std::numeric_limits<float>::max();
        bool tIntersects = true;

        for (int tAxis = 0; tAxis < 3 && tIntersects; ++tAxis)
        {
            float tOrigin = tRayOrigin[tAxis];
            float tDirection = tRayDirection[tAxis];
            float tMinBound = tBoxMin[tAxis];
            float tMaxBound = tBoxMax[tAxis];

            if (std::fabs(tDirection) < 1e-8f)
            {
                if (tOrigin < tMinBound || tOrigin > tMaxBound)
                {
                    tIntersects = false;
                }
            }
            else
            {
                float tInvDirection = 1.0f / tDirection;
                float tSlabEntry = (tMinBound - tOrigin) * tInvDirection;
                float tSlabExit = (tMaxBound - tOrigin) * tInvDirection;

                if (tSlabEntry > tSlabExit)
                {
                    std::swap(tSlabEntry, tSlabExit);
                }

                tEntry = std::max(tEntry, tSlabEntry);
                tExit = std::min(tExit, tSlabExit);

                if (tEntry > tExit)
                {
                    tIntersects = false;
                }
            }
        }

        if (tIntersects && tEntry < tClosestDistance)
        {
            tClosestDistance = tEntry;
            tClosestPlayerId = tPlayerId;
            tHasHit = true;
        }
    }

    if (!tHasHit)
    {
        return std::nullopt;
    }

    auto tIterator = mRemotePlayers.find(tClosestPlayerId);
    std::string tName = (tIterator != mRemotePlayers.end() && !tIterator->second.username.empty())
        ? tIterator->second.username
        : ("Player" + std::to_string(tClosestPlayerId));

    return TargetInfo{ TargetKind::ePlayer, tClosestPlayerId, tName };
}

std::shared_ptr<ChatPanel> WorldScene::ensureChatPanel(uint64_t pTargetId, const std::string& pTargetUsername)
{
    auto tIterator = mChatPanels.find(pTargetId);

    if (tIterator != mChatPanels.end())
    {
        return tIterator->second;
    }

    auto tPanel = std::make_shared<ChatPanel>(pTargetId, pTargetUsername);
    tPanel->isOpen = false; // stays hidden until the player actually opens it
    mChatPanels[pTargetId] = tPanel;
    mEngine.getGuiLayer().addPanel(tPanel);
    return tPanel;
}

void WorldScene::openChatWith(uint64_t pTargetId, const std::string& pTargetUsername)
{
    auto tPanel = ensureChatPanel(pTargetId, pTargetUsername);
    tPanel->show();
    ImGui::SetWindowFocus(tPanel->name.c_str());
}

void WorldScene::onDirectMessageReceived(uint64_t pFromPlayerId, const std::string& pFromUsername, const std::string& pText)
{
    // Deliberately doesn't call openChatWith - an incoming whisper queues
    // into the (possibly hidden) panel's history and flags it unread
    // instead of popping the window open on top of whatever the player is
    // doing; renderUnreadChatIndicators() surfaces it as a blinking entry
    // instead.
    auto tPanel = ensureChatPanel(pFromPlayerId, pFromUsername);
    tPanel->appendIncoming(pFromUsername, pText);
}

void WorldScene::renderUnreadChatIndicators()
{
    std::vector<std::shared_ptr<ChatPanel>> tUnreadPanels;

    for (auto& [tTargetId, tPanel] : mChatPanels)
    {
        if (tPanel->hasUnread())
        {
            tUnreadPanels.push_back(tPanel);
        }
    }

    if (tUnreadPanels.empty())
    {
        return;
    }

    // Simple on/off blink (not a smooth pulse) - alternates the entry's
    // color on a fixed period so it visibly catches the eye without
    // affecting whether it can be clicked.
    bool tBlinkOn = std::fmod(ImGui::GetTime(), 1.0) < 0.5;
    ImVec4 tBlinkColor = tBlinkOn ? UiTheme::kDanger : UiTheme::kDangerDim;

    ImVec2 tDisplaySize = ImGui::GetIO().DisplaySize;
    ImVec2 tWindowLocalPos(tDisplaySize.x - 220.0f, 60.0f);

    ImGui::SetNextWindowPos(toGlobalImGuiPos(tWindowLocalPos));
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags tFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing;

    if (ImGui::Begin("##UnreadChatIndicators", nullptr, tFlags))
    {
        for (const auto& tPanel : tUnreadPanels)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, tBlinkColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, tBlinkColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, tBlinkColor);

            std::string tLabel = "New message: " + tPanel->getTargetUsername();

            if (ImGui::Button(tLabel.c_str(), ImVec2(200, 0)))
            {
                openChatWith(tPanel->getTargetPlayerId(), tPanel->getTargetUsername());
            }

            ImGui::PopStyleColor(3);
        }
    }

    ImGui::End();
}
