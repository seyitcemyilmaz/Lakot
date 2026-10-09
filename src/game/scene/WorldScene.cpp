#include "WorldScene.h"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <imgui.h>

#include "../Engine.h"
#include "../gui/AttachmentPanel.h"
#include "../gui/PanelLoader.h"
#include "../gui/MapImageFactory.h"

#include "MovementRules.h"

#include "LoginScene.h"
#include "world/WorldViewConstants.h"

namespace
{
    constexpr float kMinPositionDeltaSq = 0.01f;
    constexpr float kMinYawDelta = 0.01f;

    constexpr float kNameplatePixelOffset = 30.0f;

    constexpr float kSpeechBubbleLineGap = 4.0f;

    constexpr float kClickDragThresholdPixels = 6.0f;

    constexpr double kComboWindowSeconds = 1.0;

    constexpr float kHealthBarWidth = 60.0f;
    constexpr float kHealthBarHeight = 5.0f;
    constexpr float kDamageFontScale = 1.4f;

    constexpr float kFogStart = 140.0f;

    const glm::vec3 kSkyColor(0.64f, 0.76f, 0.88f);

    const glm::vec3 kWaterColor(0.20f, 0.42f, 0.62f);

    const glm::vec3 kDestinationMarkerSize(1.4f, 0.15f, 1.4f);
    const glm::vec3 kDestinationMarkerColor(0.85f, 0.65f, 0.25f);
}

using namespace lakot;

WorldScene::~WorldScene()
{

}

WorldScene::WorldScene(Engine& pEngine, const std::optional<InitialSpawnState>& pSpawnState, const std::string& pUsername)
    : Scene(pEngine)
    , mInitialSpawnState(pSpawnState)
    , mLocalUsername(pUsername)
{

}

void WorldScene::setCharacterState(uint64_t pCharacterId, const CharacterStats& pStats, std::vector<OwnedItem> pItems)
{
    mLocalCharacterId = pCharacterId;
    mLocalStats = pStats;
    mLocalItems = std::move(pItems);
}

void WorldScene::enter()
{
    SDL_Log("WorldScene: enter");

    GuiLayer& tGui = mEngine.getGuiLayer();
    tGui.clearPanels();
    PanelLoader::load(tGui, PanelContext::World);

    mCamera = std::make_unique<ThirdPersonCamera>();

    const MapCatalog& tCatalog = mEngine.getMapCatalog();

    if (mInitialSpawnState)
    {
        teleportLocalPosition(glm::vec3(mInitialSpawnState->x, mInitialSpawnState->y, mInitialSpawnState->z));
        mCamera->setYaw(glm::degrees(mInitialSpawnState->yaw));
    }
    else
    {
        const MapSpawn& tSpawn = tCatalog.getStartMap(1)->getSpawn();
        teleportLocalPosition(glm::vec3(tSpawn.x, 0.0f, tSpawn.z));
        mCamera->setYaw(glm::degrees(tSpawn.yaw));
    }

    mCamera->setFar(WorldView::kViewDistance + 40.0f);
    mEngine.setClearColor(kSkyColor);

    ShaderManager& tShaderManager = mEngine.getShaderManager();

    const std::string tFogFunction = R"(
        uniform vec3 uCameraPosition;
        uniform vec3 uFogColor;
        uniform float uFogStart;
        uniform float uFogEnd;

        vec3 applyFog(vec3 pColor, vec3 pWorldPosition)
        {
            float tDistance = distance(pWorldPosition, uCameraPosition);
            float tFog = clamp((tDistance - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);
            return mix(pColor, uFogColor, tFog);
        }
    )";

    const std::string tLightFunction = R"(
        uniform vec3 uLightDirection;

        vec3 applyLight(vec3 pColor, vec3 pNormal)
        {
            float tDiffuse = max(dot(normalize(pNormal), -uLightDirection), 0.0);
            return pColor * (0.52 + 0.62 * tDiffuse);
        }
    )";

    std::string tBoxInstancedVertexSource = R"(
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aOffset;
        layout (location = 2) in vec3 aSize;
        layout (location = 3) in vec3 aColor;

        uniform mat4 uViewProjection;

        out vec3 vColor;
        out vec3 vWorldPosition;

        void main()
        {
            vWorldPosition = (aPos * aSize) + aOffset;
            gl_Position = uViewProjection * vec4(vWorldPosition, 1.0);
            vColor = aColor;
        }
    )";

    std::string tBoxInstancedFragmentSource = R"(
        in vec3 vColor;
        in vec3 vWorldPosition;
        out vec4 FragColor;
    )" + tFogFunction + tLightFunction + R"(
        void main()
        {
            vec3 tNormal = cross(dFdx(vWorldPosition), dFdy(vWorldPosition));
            FragColor = vec4(applyFog(applyLight(vColor, tNormal), vWorldPosition), 1.0);
        }
    )";

    std::string tTerrainVertexSource = R"(
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec3 aColor;

        uniform mat4 uViewProjection;

        out vec3 vWorldPosition;
        out vec3 vNormal;
        out vec3 vColor;

        void main()
        {
            vWorldPosition = aPos;
            vNormal = aNormal;
            vColor = aColor;
            gl_Position = uViewProjection * vec4(aPos, 1.0);
        }
    )";

    std::string tTerrainFragmentSource = R"(
        in vec3 vWorldPosition;
        in vec3 vNormal;
        in vec3 vColor;
        out vec4 FragColor;
    )" + tFogFunction + tLightFunction + R"(
        void main()
        {
            FragColor = vec4(applyFog(applyLight(vColor, vNormal), vWorldPosition), 1.0);
        }
    )";

    std::string tModelVertexSource = R"(
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aUv;
        layout (location = 3) in vec3 aOffset;
        layout (location = 4) in vec3 aYawScaleStretch;

        uniform mat4 uViewProjection;

        out vec3 vWorldPosition;
        out vec3 vNormal;
        out vec2 vUv;

        void main()
        {
            float tCos = cos(aYawScaleStretch.x);
            float tSin = sin(aYawScaleStretch.x);
            vec3 tScale = vec3(aYawScaleStretch.y * aYawScaleStretch.z, aYawScaleStretch.y, aYawScaleStretch.y);
            mat3 tRotation = mat3(tCos, 0.0, -tSin,
                                  0.0,  1.0,  0.0,
                                  tSin, 0.0,  tCos);

            vWorldPosition = tRotation * (aPos * tScale) + aOffset;
            vNormal = tRotation * aNormal;
            vUv = aUv;
            gl_Position = uViewProjection * vec4(vWorldPosition, 1.0);
        }
    )";

    std::string tModelFragmentSource = R"(
        in vec3 vWorldPosition;
        in vec3 vNormal;
        in vec2 vUv;
        out vec4 FragColor;

        uniform sampler2D uTexture;
        uniform vec3 uTint = vec3(1.0);
        uniform float uGlow = 0.0;
    )" + tFogFunction + tLightFunction + R"(
        void main()
        {
            vec3 tColor = texture(uTexture, vUv).rgb * uTint;
            FragColor = vec4(applyFog(applyLight(tColor, vNormal) + tColor * uGlow, vWorldPosition), 1.0);
        }
    )";

    std::string tSkinnedVertexSource = R"(
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aUv;
        layout (location = 3) in ivec4 aJoints;
        layout (location = 4) in vec4 aWeights;

        uniform mat4 uViewProjection;
        uniform mat4 uModel;
        uniform mat4 uBones[64];

        out vec3 vWorldPosition;
        out vec3 vNormal;
        out vec2 vUv;

        void main()
        {
            mat4 tSkin = uBones[aJoints.x] * aWeights.x + uBones[aJoints.y] * aWeights.y
                       + uBones[aJoints.z] * aWeights.z + uBones[aJoints.w] * aWeights.w;
            mat4 tWorld = uModel * tSkin;

            vWorldPosition = (tWorld * vec4(aPos, 1.0)).xyz;
            vNormal = mat3(tWorld) * aNormal;
            vUv = aUv;
            gl_Position = uViewProjection * vec4(vWorldPosition, 1.0);
        }
    )";

    if (!tShaderManager.hasProgram("world_box"))
    {
        tShaderManager.createProgram("world_box", tBoxInstancedVertexSource, tBoxInstancedFragmentSource);
    }

    if (!tShaderManager.hasProgram("world_terrain"))
    {
        tShaderManager.createProgram("world_terrain", tTerrainVertexSource, tTerrainFragmentSource);
    }

    if (!tShaderManager.hasProgram("world_model"))
    {
        tShaderManager.createProgram("world_model", tModelVertexSource, tModelFragmentSource);
    }

    if (!tShaderManager.hasProgram("world_skinned"))
    {
        tShaderManager.createProgram("world_skinned", tSkinnedVertexSource, tModelFragmentSource);
    }

    mBoxContainer = std::make_unique<BoxContainer>();
    mBoxContainer->initialize();

    mItemAppearance = std::make_unique<ItemAppearance>(mEngine.getAssetManager(), mEngine.getNetworkManager().getInventoryController());
    mItemIcons = std::make_unique<ItemIconRenderer>(*mItemAppearance, mEngine.getNetworkManager().getInventoryController(),
                                                    *tShaderManager.getProgram("world_skinned"), mEngine.getRmlUiLayer());
    mAttachments = std::make_unique<EquipmentAttachments>();
    std::string tAttachmentError;

    if (!mAttachments->load(mEngine.getAssetManager().resolve("attachments.json"), tAttachmentError))
    {
        SDL_Log("WorldScene: %s", tAttachmentError.c_str());
    }

#if defined(LAKOT_DEV_TOOLS)
    mAttachmentPanel = std::make_shared<AttachmentPanel>(*mAttachments, std::vector<std::string>{
        LAKOT_SOURCE_DATA_DIR "/attachments.json", mEngine.getAssetManager().resolve("attachments.json") });
    tGui.addPanel(mAttachmentPanel);
#endif

    mEntityRenderer = std::make_unique<EntityRenderer>(*mItemAppearance, *mAttachments);

    mEntities = std::make_unique<WorldEntities>(mEngine.getAssetManager(), mEngine.getMonsterCatalog(), mEngine.getMotionLibrary(), *mItemAppearance);
    mEntities->setLocalCharacterId(mLocalCharacterId);
    mEntities->setLocal(mLocalUsername, mLocalPosition, mEngine.getFixedDeltaTime());
    mEntities->setLocalEquipment(mLocalItems);

    mPicker = std::make_unique<WorldPicker>(mEngine.getWindow(), *mCamera, *mEntities);
    mAutoWalk = std::make_unique<AutoWalk>(*mEntities);

    mHud = std::make_unique<WorldHud>(mEngine.getRmlUiLayer(), mEngine.getNetworkManager(), mEngine.getDisplaySettings(),
                                      mEngine.getWindow(), mLocalUsername, mLocalStats, mLocalItems, *mItemIcons);

    mHud->setLocalItemsCallback(
    [this](const std::vector<OwnedItem>& pItems)
    {
        mEntities->setLocalEquipment(pItems);
    });

    mHud->setLogoutCallback(
    [this]()
    {
        mEngine.getNetworkManager().logout();
        mEngine.getSceneManager().setNextScene(std::make_unique<LoginScene>(mEngine));
    });

    mHud->setLocalSayCallback(
    [this](const std::string& pText)
    {
        mEntities->showSpeech(WorldEntities::kLocalId, pText);
    });

    loadMap(mInitialSpawnState ? mInitialSpawnState->mapId : tCatalog.getStartMap(1)->getId());

    mLocalCharacterBox = std::make_unique<BoxContainer>();
    mLocalCharacterBox->initialize();

    WorldController& tWorldController = mEngine.getNetworkManager().getWorldController();

    tWorldController.setWorldSnapshotCallback(
    [this](const WorldController::WorldSnapshot& pSnapshot)
    {
        this->onWorldSnapshot(pSnapshot);
    });

    tWorldController.setMapChangedCallback(
    [this](const WorldController::MapSnapshot& pSnapshot)
    {
        this->onMapChanged(pSnapshot);
    });

    tWorldController.setPickupResultCallback(
    [this](services::world::PickupResult pResult)
    {
        this->onPickupResult(pResult);
    });

    mEngine.getNetworkManager().getChatController().setChatMessageReceivedCallback(
    [this](uint64_t pFromPlayerId, const std::string& pFromUsername, const std::string& pText, bool pIsGlobal)
    {
        mHud->onWorldChatMessage(pFromUsername, pText, pIsGlobal);

        if (!pIsGlobal)
        {
            mEntities->showSpeech(pFromPlayerId, pText);
        }
    });
}

void WorldScene::exit()
{
    SDL_Log("WorldScene: exit");

    mEngine.getNetworkManager().getWorldController().clearCallbacks();
    mEngine.getNetworkManager().getChatController().clearCallbacks();
    mEngine.getNetworkManager().getInventoryController().clearCallbacks();

    endOrbit();

    mEngine.getGuiLayer().clearPanels();
    mAttachmentPanel.reset();

    mEngine.setClearColor(glm::vec3(0.4f, 0.4f, 0.4f));
}

void WorldScene::update(double pDeltaTime)
{
    if (!mCamera)
    {
        return;
    }

    if (mIsOrbiting && !(SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK))
    {
        endOrbit();
    }

    updateAttack(pDeltaTime);
    updateLocalMovement(pDeltaTime);
    updatePickup();
    mCameraBob.update(glm::length(glm::vec2(mLocalPosition.x - mPreviousLocalPosition.x, mLocalPosition.z - mPreviousLocalPosition.z)), pDeltaTime);

    mStateSendAccumulator += pDeltaTime;

    if (mStateSendAccumulator >= WorldView::kStateSendInterval)
    {
        mStateSendAccumulator = 0.0;
        sendLocalPlayerState();
    }

    mEntities->update(pDeltaTime);
    mEntities->setLocal(mLocalUsername, mLocalPosition, pDeltaTime);

    for (const DamageEvent& tEvent : mEntities->takeDamageEvents())
    {
        mDamageNumbers.add(tEvent.targetId, tEvent.damage, tEvent.isAgainstLocal, tEvent.isByLocal);
    }

    mDamageNumbers.update(static_cast<float>(pDeltaTime));
    mHud->setSafeZone(mMap && mMap->isSafe(mLocalPosition.x, mLocalPosition.z));

    glm::vec3 tFacing = mCamera->getFrontVector();
    float tFacingDegrees = glm::degrees(std::atan2(tFacing.x, -tFacing.z));

    std::vector<glm::vec3> tOthers;
    tOthers.reserve(mEntities->getAll().size());

    for (const auto& [tId, tEntity] : mEntities->getAll())
    {
        if (tId != WorldEntities::kLocalId && tEntity.type != EntityType::eGroundItem)
        {
            tOthers.push_back(WorldEntities::getPosition(tEntity));
        }
    }

    mHud->update(pDeltaTime, mLocalPosition, tFacingDegrees, tOthers);
}

void WorldScene::updatePickup()
{
    if (mPendingPickupId == 0)
    {
        return;
    }

    auto tPosition = mEntities->findPosition(mPendingPickupId);

    if (!tPosition)
    {
        mPendingPickupId = 0;
        return;
    }

    constexpr float kPickupReach = 2.8f;

    if (glm::distance(glm::vec2(tPosition->x, tPosition->z), glm::vec2(mLocalPosition.x, mLocalPosition.z)) <= kPickupReach)
    {
        mEngine.getNetworkManager().getWorldController().sendPickup(mPendingPickupId);
        mPendingPickupId = 0;
        mAutoWalk->stop();
    }
    else if (!mAutoWalk->isActive())
    {
        mPendingPickupId = 0;
    }
}

void WorldScene::onPickupResult(services::world::PickupResult pResult)
{
    switch (pResult)
    {
        case services::world::PICKUP_RESULT_NOT_OWNER: mHud->showNotice("This belongs to someone else for now."); break;
        case services::world::PICKUP_RESULT_TOO_FAR: mHud->showNotice("Too far away to pick that up."); break;
        case services::world::PICKUP_RESULT_INVENTORY_FULL: mHud->showNotice("Your bag is full."); break;
        case services::world::PICKUP_RESULT_NOT_FOUND: mHud->showNotice("That item is gone."); break;
        case services::world::PICKUP_RESULT_DEAD: mHud->showNotice("You cannot do that while dead."); break;
        default: break;
    }
}

void WorldScene::updateAttack(double pDeltaTime)
{
    mAttackCooldown -= pDeltaTime;
    mComboResetTimer -= pDeltaTime;
    mAttackRootTimer -= pDeltaTime;

    if (!mIsAttackHeld)
    {
        mHasWarnedSafeZone = false;
        return;
    }

    if (mIsLocalDead || mAttackCooldown > 0.0 || !mMap)
    {
        return;
    }

    if (mMap->isSafe(mLocalPosition.x, mLocalPosition.z))
    {
        if (!mHasWarnedSafeZone)
        {
            mHud->showNotice("You cannot fight here.");
            mHasWarnedSafeZone = true;
        }

        return;
    }

    if (mComboResetTimer <= 0.0)
    {
        mCombo = 0;
    }

    const WorldEntity* tLocal = mEntities->getLocal();
    float tFacing = tLocal ? tLocal->facing : 0.0f;

    mEngine.getNetworkManager().getWorldController().sendAttack(tFacing, mCombo);
    mEntities->playLocalAttack(mCombo);
    mAutoWalk->stop();

    mCombo = (mCombo + 1) % 3;
    mAttackCooldown = WorldView::kAttackIntervalSeconds;
    mComboResetTimer = kComboWindowSeconds;
    mAttackRootTimer = WorldView::kAttackIntervalSeconds;
}

glm::vec3 WorldScene::slideLocal(glm::vec3 pPosition, glm::vec2 pStep) const
{
    const MapData& tField = *mMap;

    if (tField.isMoveAllowed(pPosition.x, pPosition.z, pPosition.x + pStep.x, pPosition.z + pStep.y))
    {
        pPosition.x += pStep.x;
        pPosition.z += pStep.y;
    }
    else if (tField.isMoveAllowed(pPosition.x, pPosition.z, pPosition.x + pStep.x, pPosition.z))
    {
        pPosition.x += pStep.x;
    }
    else if (tField.isMoveAllowed(pPosition.x, pPosition.z, pPosition.x, pPosition.z + pStep.y))
    {
        pPosition.z += pStep.y;
    }

    return pPosition;
}

void WorldScene::updateLocalMovement(double pDeltaTime)
{
    glm::vec2 tRootMotion = mEntities->takeLocalRootMotion();

    if (mIsLocalDead || mAttackRootTimer > 0.0)
    {
        mPreviousLocalPosition = mLocalPosition;

        if (!mIsLocalDead && mMap && glm::length(tRootMotion) > 0.0f)
        {
            mLocalPosition = slideLocal(mLocalPosition, tRootMotion);
            mLocalPosition.y = mMap->getHeightAt(mLocalPosition.x, mLocalPosition.z) + WorldView::kPlayerBoxHalfExtents.y;
        }

        return;
    }

    float tSpeed = MovementRules::kWalkSpeed * static_cast<float>(pDeltaTime);
    glm::vec3 tMovement(0.0f);

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

    mPreviousLocalPosition = mLocalPosition;

    glm::vec3 tPosition = glm::length(tRootMotion) > 0.0f ? slideLocal(mLocalPosition, tRootMotion) : mLocalPosition;

    bool tIsManual = glm::length(tMovement) > 0.0f;

    if (tIsManual)
    {
        mAutoWalk->stop();
    }
    else if (mAutoWalk->isActive())
    {
        glm::vec2 tNext = mAutoWalk->advance(*mMap, glm::vec2(tPosition.x, tPosition.z), tSpeed, pDeltaTime);
        tPosition.x = tNext.x;
        tPosition.z = tNext.y;
    }

    if (tIsManual)
    {
        tMovement = glm::normalize(tMovement) * tSpeed;
        tPosition = slideLocal(tPosition, glm::vec2(tMovement.x, tMovement.z));
    }

    tPosition.y = mMap->getHeightAt(tPosition.x, tPosition.z) + WorldView::kPlayerBoxHalfExtents.y;

    mLocalPosition = tPosition;
}

void WorldScene::render()
{
    int tWidth;
    int tHeight;
    SDL_GetWindowSizeInPixels(mEngine.getWindow(), &tWidth, &tHeight);
    mViewportWidth = tWidth;
    mViewportHeight = tHeight;

    double tAlpha = mEngine.getInterpolationAlpha();
    double tExtraSeconds = tAlpha * mEngine.getFixedDeltaTime();

    glm::vec3 tLocalRenderPosition = glm::mix(mPreviousLocalPosition, mLocalPosition, static_cast<float>(tAlpha));

    mCamera->setTargetPosition(tLocalRenderPosition + mCameraBob.getOffset(mCamera->getRightVector(), static_cast<float>(tAlpha)));
    mCamera->updateProjection(tWidth, tHeight);
    mCamera->update();

    constexpr float kEyeClearance = 1.0f;
    const glm::vec3& tEye = mCamera->getPosition();
    mCamera->setMinimumEyeHeight(mMap->getHeightAt(tEye.x, tEye.z) + kEyeClearance);
    mCamera->update();

    if (mLocalCharacterBox)
    {
        std::vector<glm::vec3> tPositions;
        std::vector<glm::vec3> tSizes;
        std::vector<glm::vec3> tColors;

        if (auto tEnd = mAutoWalk->getDestination())
        {
            tPositions.emplace_back(tEnd->x, mMap->getHeightAt(tEnd->x, tEnd->y) + kDestinationMarkerSize.y * 0.5f, tEnd->y);
            tSizes.push_back(kDestinationMarkerSize);
            tColors.push_back(kDestinationMarkerColor);
        }

        for (const auto& [tId, tEntity] : mEntities->getAll())
        {
            if (tEntity.type != EntityType::eGroundItem)
            {
                continue;
            }

            glm::vec3 tBase = WorldEntities::getPosition(tEntity, tExtraSeconds);

            if (tEntity.gold > 0)
            {
                tPositions.push_back(tBase + glm::vec3(0.0f, 0.06f, 0.0f));
                tSizes.emplace_back(0.5f, 0.12f, 0.5f);
                tColors.emplace_back(0.85f, 0.65f, 0.1f);
                tPositions.push_back(tBase + glm::vec3(0.0f, 0.18f, 0.0f));
                tSizes.emplace_back(0.3f, 0.12f, 0.3f);
                tColors.emplace_back(1.0f, 0.82f, 0.2f);
            }
            else if (!mItemAppearance->find({ tEntity.itemTemplateId, 0 }))
            {
                tPositions.push_back(tBase + glm::vec3(0.0f, 0.15f, 0.0f));
                tSizes.emplace_back(0.3f, 0.3f, 0.3f);
                tColors.emplace_back(0.45f, 0.75f, 1.0f);
            }
        }

        mLocalCharacterBox->setBoxes(tPositions, tSizes, tColors);
    }

    glEnable(GL_DEPTH_TEST);

    ShaderManager& tShaderManager = mEngine.getShaderManager();

    auto tSetWorldUniforms = [&](ShaderProgram* pShader)
    {
        pShader->setMat4("uViewProjection", mCamera->getViewProjectionMatrix());
        pShader->setVec3("uCameraPosition", mCamera->getPosition());
        pShader->setVec3("uFogColor", kSkyColor);
        pShader->setFloat("uFogStart", kFogStart);
        pShader->setFloat("uFogEnd", WorldView::kViewDistance);
        pShader->setVec3("uLightDirection", WorldView::kLightDirection);
    };

    ShaderProgram* tTerrainShader = tShaderManager.getProgram("world_terrain");

    if (tTerrainShader && mTerrain)
    {
        tTerrainShader->bind();
        tSetWorldUniforms(tTerrainShader);

        mTerrain->getVertexArrayObject().bind();
        mTerrain->drawChunksNear(mCamera->getPosition(), WorldView::kViewDistance);

        tTerrainShader->unbind();
    }

    ShaderProgram* tModelShader = tShaderManager.getProgram("world_model");

    if (tModelShader)
    {
        tModelShader->bind();
        tSetWorldUniforms(tModelShader);
        tModelShader->setInt("uTexture", 0);

        for (const std::shared_ptr<Model>& tModel : mMapModels)
        {
            tModel->draw(0);
        }

        tModelShader->unbind();
    }

    ShaderProgram* tSkinnedShader = tShaderManager.getProgram("world_skinned");

    if (tSkinnedShader)
    {
        tSkinnedShader->bind();
        tSetWorldUniforms(tSkinnedShader);
        tSkinnedShader->setInt("uTexture", 0);

        mEntityRenderer->draw(*tSkinnedShader, *mEntities, tExtraSeconds);

        tSkinnedShader->unbind();
    }

    ShaderProgram* tBoxShader = tShaderManager.getProgram("world_box");

    if (tBoxShader)
    {
        tBoxShader->bind();
        tSetWorldUniforms(tBoxShader);

        for (BoxContainer* tBoxes : { mBoxContainer.get(), mLocalCharacterBox.get() })
        {
            if (tBoxes && tBoxes->getInstanceCount() > 0)
            {
                tBoxes->getVertexArrayObject().bind();
                glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr, tBoxes->getInstanceCount());
            }
        }

        tBoxShader->unbind();
    }

    for (const auto& [tId, tEntity] : mEntities->getAll())
    {
        bool tShowsHealth = tEntity.type != EntityType::eNpc && tEntity.type != EntityType::eGroundItem && tId != WorldEntities::kLocalId && !tEntity.isDead
                         && (tEntity.type == EntityType::eMonster || tEntity.healthPercent < 100);

        std::string tTagName = tEntity.level > 0 ? "Lv " + std::to_string(tEntity.level) + " " + tEntity.name : tEntity.name;

        renderNameplate(WorldEntities::getPosition(tEntity, tExtraSeconds) + tEntity.getHeadTopOffset(), tTagName, tEntity.speechText,
                        getNameColor(tEntity), tShowsHealth ? static_cast<int>(tEntity.healthPercent) : -1);
    }

    renderDamageNumbers(tExtraSeconds);

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
                mHud->openWhisper(mContextMenuTarget->id, mContextMenuTarget->name);
                ImGui::CloseCurrentPopup();
            }

            ImGui::BeginDisabled();
            ImGui::MenuItem("Trade");
            ImGui::EndDisabled();
        }

        ImGui::EndPopup();
    }
    else
    {
        mContextMenuTarget.reset();
    }
}

bool WorldScene::handleEvent(SDL_Event* pEvent)
{
    if (pEvent->type == SDL_EVENT_KEY_DOWN || pEvent->type == SDL_EVENT_KEY_UP)
    {
        bool tIsPressed = (pEvent->type == SDL_EVENT_KEY_DOWN);

        if (tIsPressed && ImGui::GetIO().WantCaptureKeyboard)
        {
            return false;
        }

        switch (pEvent->key.key)
        {
            case SDLK_W: mIsMovingForward = tIsPressed;  return true;
            case SDLK_S: mIsMovingBackward = tIsPressed; return true;
            case SDLK_A: mIsMovingLeft = tIsPressed;     return true;
            case SDLK_D: mIsMovingRight = tIsPressed;    return true;
            case SDLK_SPACE: mIsAttackHeld = tIsPressed; return true;
            case SDLK_F10:
                if (tIsPressed && mAttachmentPanel)
                {
                    mAttachmentPanel->isOpen = !mAttachmentPanel->isOpen;
                }
                return true;
            default: break;
        }

        return tIsPressed && mHud->handleKeyDown(pEvent->key);
    }

    if (pEvent->type == SDL_EVENT_WINDOW_FOCUS_LOST)
    {
        mIsAttackHeld = false;
        endOrbit();
        return false;
    }

    if (pEvent->type == SDL_EVENT_MOUSE_BUTTON_DOWN && pEvent->button.button == SDL_BUTTON_RIGHT)
    {
        if (ImGui::GetIO().WantCaptureMouse)
        {
            return false;
        }

        mHud->closeWorldChat();

        mIsOrbiting = true;
        mOrbitDragDistance = 0.0f;
        mRightClickDownPos = glm::vec2(pEvent->button.x, pEvent->button.y);
        SDL_SetWindowRelativeMouseMode(mEngine.getWindow(), true);
        return true;
    }

    if (pEvent->type == SDL_EVENT_MOUSE_BUTTON_DOWN && pEvent->button.button == SDL_BUTTON_LEFT && mHud->isSystemMenuOpen())
    {
        return true;
    }

    if (pEvent->type == SDL_EVENT_MOUSE_BUTTON_DOWN && pEvent->button.button == SDL_BUTTON_LEFT)
    {
        if (!ImGui::GetIO().WantCaptureMouse)
        {
            mHud->closeWorldChat();

            glm::vec2 tPixel(pEvent->button.x, pEvent->button.y);

            if (auto tTarget = mPicker->pickTarget(tPixel))
            {
                mAutoWalk->follow(tTarget->id);
                mPendingPickupId = tTarget->type == EntityType::eGroundItem ? tTarget->id : 0;
                return true;
            }

            mPendingPickupId = 0;

            if (auto tGround = mPicker->pickGround(*mMap, tPixel))
            {
                mAutoWalk->walkTo(*mMap, glm::vec2(mLocalPosition.x, mLocalPosition.z), glm::vec2(tGround->x, tGround->z));
                return true;
            }
        }
    }

    if (pEvent->type == SDL_EVENT_MOUSE_BUTTON_UP && pEvent->button.button == SDL_BUTTON_RIGHT)
    {
        bool tWasOrbiting = mIsOrbiting;
        endOrbit();

        if (tWasOrbiting && mOrbitDragDistance < kClickDragThresholdPixels && !mHud->isSystemMenuOpen())
        {
            if (auto tTarget = mPicker->pickTarget(mRightClickDownPos); tTarget && tTarget->type == EntityType::ePlayer)
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

void WorldScene::onSessionResumed()
{
    mAutoWalk->stop();

    mEntities->clearRemote();

    mIsMovingForward = false;
    mIsMovingBackward = false;
    mIsMovingLeft = false;
    mIsMovingRight = false;
    mIsAttackHeld = false;
    endOrbit();
    mContextMenuTarget.reset();

    mHasSentInitialState = false;
}

void WorldScene::onWorldSnapshot(const WorldController::WorldSnapshot& pSnapshot)
{
    mEntities->applySnapshot(pSnapshot);

    if (pSnapshot.selfVitals)
    {
        mLocalKingdom = pSnapshot.selfVitals->kingdom;
        mIsLocalDead = pSnapshot.selfVitals->health <= 0;
        mHud->onSelfVitals(*pSnapshot.selfVitals);
    }

    if (pSnapshot.selfCorrection && mCamera)
    {
        const auto& tCorrection = *pSnapshot.selfCorrection;

        teleportLocalPosition(glm::vec3(tCorrection.x, tCorrection.y, tCorrection.z));

        mLastSentPosition = mLocalPosition;

        mAutoWalk->onRepositioned(*mMap, glm::vec2(mLocalPosition.x, mLocalPosition.z));

        mHasSentInitialState = true;
    }
}

void WorldScene::onMapChanged(const WorldController::MapSnapshot& pSnapshot)
{
    mEntities->clearRemote();
    mHud->clearWorldChat();

    if (mCamera)
    {
        teleportLocalPosition(glm::vec3(pSnapshot.x, pSnapshot.y, pSnapshot.z));
        mCamera->setYaw(glm::degrees(pSnapshot.yaw));
    }

    mHasSentInitialState = false;

    mAutoWalk->stop();

    if (!mMap || mMap->getId() != pSnapshot.mapId)
    {
        loadMap(pSnapshot.mapId);
    }
}

void WorldScene::teleportLocalPosition(const glm::vec3& pPosition)
{
    mLocalPosition = pPosition;
    mPreviousLocalPosition = pPosition;

    if (mEntities)
    {
        mEntities->teleportLocal(pPosition);
    }
}

void WorldScene::loadMap(uint32_t pMapId)
{
    const MapCatalog& tCatalog = mEngine.getMapCatalog();

    mMap = tCatalog.getMap(pMapId);

    if (!mMap)
    {
        SDL_Log("WorldScene: unknown map %u, client data may be out of date", pMapId);
        mMap = tCatalog.getStartMap(0);
    }

    mTerrain = std::make_unique<Terrain>(*mMap);
    mTerrain->initialize();

    std::vector<glm::vec3> tPositions;
    std::vector<glm::vec3> tSizes;
    std::vector<glm::vec3> tColors;

    tPositions.reserve(mMap->getObjects().size() + 1);
    tSizes.reserve(tPositions.capacity());
    tColors.reserve(tPositions.capacity());

    std::unordered_map<std::shared_ptr<Model>, std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>>> tModelInstances;
    AssetManager& tAssets = mEngine.getAssetManager();

    for (const MapObject& tObject : mMap->getObjects())
    {
        if (std::shared_ptr<Model> tModel = tObject.model.empty() ? nullptr : tAssets.getModel("models/" + tObject.model + ".glb"))
        {
            auto& [tOffsets, tParams] = tModelInstances[tModel];
            float tGround = mMap->getHeightAt(tObject.x, tObject.z);
            tOffsets.emplace_back(tObject.x, tGround + tObject.elevation, tObject.z);
            tParams.emplace_back(tObject.yaw, tObject.scale, tObject.stretch);
            continue;
        }

        if (tObject.isHidden)
        {
            continue;
        }

        tPositions.push_back(WorldPicker::getObjectCenter(*mMap, tObject));
        tSizes.emplace_back(tObject.sizeX, tObject.sizeY, tObject.sizeZ);
        tColors.emplace_back(tObject.color[0], tObject.color[1], tObject.color[2]);
    }

    constexpr float kWaterThickness = 0.1f;
    const float tWorldSize = mMap->getWorldSize();

    tPositions.emplace_back(0.0f, mMap->getWaterLevel() - kWaterThickness * 0.5f, 0.0f);
    tSizes.emplace_back(tWorldSize, kWaterThickness, tWorldSize);
    tColors.push_back(kWaterColor);

    mBoxContainer->setBoxes(tPositions, tSizes, tColors);

    mMapModels.clear();

    for (const auto& [tModel, tInstances] : tModelInstances)
    {
        tModel->setInstances(tInstances.first, tInstances.second);
        mMapModels.push_back(tModel);
    }

    constexpr int kMapImageSize = 1024;

    std::string tImageName = "map-" + std::to_string(mMap->getId());
    MapImage tImage = MapImageFactory::create(*mMap, kMapImageSize);
    mEngine.getRmlUiLayer().registerGeneratedImage(tImageName, tImage.size, tImage.size, std::move(tImage.rgba));

    mHud->setMap(tImageName, mMap->getName(), mMap->getWorldSize());

    SDL_Log("WorldScene: map %u - %s", mMap->getId(), mMap->getName().c_str());
}

void WorldScene::sendLocalPlayerState()
{
    if (!mCamera)
    {
        return;
    }

    const glm::vec3& tPosition = mLocalPosition;
    const glm::vec3& tFront = mCamera->getFrontVector();
    float tYaw = std::atan2(tFront.z, tFront.x);

    if (mHasSentInitialState)
    {
        glm::vec3 tDelta = tPosition - mLastSentPosition;
        float tPositionDeltaSq = glm::dot(tDelta, tDelta);
        float tYawDelta = std::fabs(std::remainder(tYaw - mLastSentYaw, glm::two_pi<float>()));

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

std::optional<ImVec2> WorldScene::projectToScreen(const glm::vec3& pWorldPosition) const
{
    if (!mCamera)
    {
        return std::nullopt;
    }

    glm::vec4 tClip = mCamera->getViewProjectionMatrix() * glm::vec4(pWorldPosition, 1.0f);

    if (tClip.w <= 0.01f)
    {
        return std::nullopt;
    }

    glm::vec3 tNdc = glm::vec3(tClip) / tClip.w;

    if (tNdc.x < -1.2f || tNdc.x > 1.2f || tNdc.y < -1.2f || tNdc.y > 1.2f)
    {
        return std::nullopt;
    }

    ImGuiIO& tIo = ImGui::GetIO();
    ImVec2 tFramebufferScale = tIo.DisplayFramebufferScale;
    if (tFramebufferScale.x <= 0.0f) tFramebufferScale.x = 1.0f;
    if (tFramebufferScale.y <= 0.0f) tFramebufferScale.y = 1.0f;

    float tPhysicalX = (tNdc.x * 0.5f + 0.5f) * static_cast<float>(mViewportWidth);
    float tPhysicalY = (1.0f - (tNdc.y * 0.5f + 0.5f)) * static_cast<float>(mViewportHeight);

    return ImVec2(tPhysicalX / tFramebufferScale.x, tPhysicalY / tFramebufferScale.y);
}

ImU32 WorldScene::getNameColor(const WorldEntity& pEntity) const
{
    if (pEntity.isDead)
    {
        return IM_COL32(150, 150, 150, 255);
    }

    switch (pEntity.type)
    {
        case EntityType::eNpc:     return IM_COL32(242, 184, 82, 255);
        case EntityType::eMonster: return IM_COL32(255, 140, 110, 255);
        case EntityType::eGroundItem: return IM_COL32(255, 224, 120, 255);
        default: break;
    }

    if (pEntity.id != WorldEntities::kLocalId && mLocalKingdom != 0 && pEntity.kingdom != 0 && pEntity.kingdom != mLocalKingdom)
    {
        return IM_COL32(255, 102, 102, 255);
    }

    return IM_COL32(255, 255, 255, 255);
}

void WorldScene::renderNameplate(const glm::vec3& pWorldPosition, const std::string& pName, const std::string& pSpeechText,
                                 ImU32 pNameColor, int pHealthPercent)
{
    if (pName.empty())
    {
        return;
    }

    std::optional<ImVec2> tScreenPos = projectToScreen(pWorldPosition);

    if (!tScreenPos)
    {
        return;
    }

    ImDrawList* tDrawList = ImGui::GetBackgroundDrawList(ImGui::GetMainViewport());

    ImVec2 tTextSize = ImGui::CalcTextSize(pName.c_str());
    ImVec2 tCentered = toGlobalImGuiPos(ImVec2(tScreenPos->x - tTextSize.x * 0.5f, tScreenPos->y - kNameplatePixelOffset - tTextSize.y));

    tDrawList->AddText(ImVec2(tCentered.x + 1.0f, tCentered.y + 1.0f), IM_COL32(0, 0, 0, 200), pName.c_str());
    tDrawList->AddText(tCentered, pNameColor, pName.c_str());

    if (pHealthPercent >= 0)
    {
        ImVec2 tBarMin = toGlobalImGuiPos(ImVec2(tScreenPos->x - kHealthBarWidth * 0.5f, tScreenPos->y - kNameplatePixelOffset + 2.0f));
        ImVec2 tBarMax(tBarMin.x + kHealthBarWidth, tBarMin.y + kHealthBarHeight);
        float tFill = kHealthBarWidth * static_cast<float>(std::clamp(pHealthPercent, 0, 100)) / 100.0f;

        tDrawList->AddRectFilled(tBarMin, tBarMax, IM_COL32(26, 26, 32, 220));
        tDrawList->AddRectFilled(tBarMin, ImVec2(tBarMin.x + tFill, tBarMax.y), IM_COL32(198, 62, 62, 255));
        tDrawList->AddRect(tBarMin, tBarMax, IM_COL32(64, 64, 76, 255));
    }

    if (!pSpeechText.empty())
    {
        float tSpeechOffset = pHealthPercent >= 0 ? kHealthBarHeight + 4.0f : 0.0f;
        ImVec2 tSpeechTextSize = ImGui::CalcTextSize(pSpeechText.c_str());
        ImVec2 tSpeechPos = toGlobalImGuiPos(ImVec2(tScreenPos->x - tSpeechTextSize.x * 0.5f,
                                                    tScreenPos->y - kNameplatePixelOffset + kSpeechBubbleLineGap + tSpeechOffset));

        tDrawList->AddText(ImVec2(tSpeechPos.x + 1.0f, tSpeechPos.y + 1.0f), IM_COL32(0, 0, 0, 200), pSpeechText.c_str());
        tDrawList->AddText(tSpeechPos, IM_COL32(255, 255, 255, 255), pSpeechText.c_str());
    }
}

void WorldScene::renderDamageNumbers(double pExtraSeconds)
{
    ImDrawList* tDrawList = ImGui::GetBackgroundDrawList(ImGui::GetMainViewport());
    for (const DamageNumber& tNumber : mDamageNumbers.getAll())
    {
        auto tEntity = mEntities->getAll().find(tNumber.entityId);

        if (tEntity == mEntities->getAll().end())
        {
            continue;
        }

        float tProgress = tNumber.age / DamageNumbers::kLifetimeSeconds;
        glm::vec3 tPosition = WorldEntities::getPosition(tEntity->second, pExtraSeconds) + tEntity->second.getHeadTopOffset()
                            + glm::vec3(0.0f, DamageNumbers::kRiseUnits * tProgress, 0.0f);

        std::optional<ImVec2> tScreenPos = projectToScreen(tPosition);

        if (!tScreenPos)
        {
            continue;
        }

        int tAlpha = static_cast<int>(255.0f * (1.0f - tProgress * tProgress));
        ImU32 tColor = tNumber.isAgainstLocal ? IM_COL32(255, 102, 102, tAlpha)
                     : tNumber.isByLocal ? IM_COL32(255, 255, 255, tAlpha)
                     : IM_COL32(200, 200, 210, tAlpha);

        ImFont* tFont = ImGui::GetFont();
        float tSize = ImGui::GetFontSize() * kDamageFontScale;
        ImVec2 tTextSize = tFont->CalcTextSizeA(tSize, FLT_MAX, 0.0f, tNumber.text.c_str());
        ImVec2 tPos = toGlobalImGuiPos(ImVec2(tScreenPos->x - tTextSize.x * 0.5f, tScreenPos->y - kNameplatePixelOffset - tTextSize.y * 2.0f));

        tDrawList->AddText(tFont, tSize, ImVec2(tPos.x + 1.0f, tPos.y + 1.0f), IM_COL32(0, 0, 0, tAlpha), tNumber.text.c_str());
        tDrawList->AddText(tFont, tSize, tPos, tColor, tNumber.text.c_str());
    }
}

ImVec2 WorldScene::toGlobalImGuiPos(const ImVec2& pWindowLocalPos) const
{
    ImGuiViewport* tViewport = ImGui::GetMainViewport();
    ImVec2 tOffset = tViewport ? tViewport->Pos : ImVec2(0.0f, 0.0f);

    return ImVec2(pWindowLocalPos.x + tOffset.x, pWindowLocalPos.y + tOffset.y);
}

bool WorldScene::isCapturingMouse() const
{
    return mIsOrbiting;
}

void WorldScene::endOrbit()
{
    mIsOrbiting = false;
    SDL_SetWindowRelativeMouseMode(mEngine.getWindow(), false);
}

bool WorldScene::onCloseRequested()
{
    mHud->handleEscape();
    return true;
}
