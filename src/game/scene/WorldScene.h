#ifndef LAKOT_WORLDSCENE_H
#define LAKOT_WORLDSCENE_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <imgui.h>

#include "Scene.h"

#include "world/AutoWalk.h"
#include "world/CameraBob.h"
#include "world/DamageNumbers.h"
#include "world/EntityRenderer.h"
#include "../gui/Panel.h"
#include "world/EquipmentAttachments.h"
#include "world/WorldEntities.h"
#include "world/WorldHud.h"
#include "world/WorldPicker.h"

#include "../graphics/camera/ThirdPersonCamera.h"

#include "../graphics/geometry/BoxContainer.h"
#include "../graphics/geometry/Terrain.h"
#include "../graphics/model/Model.h"
#include "../item/ItemAppearance.h"
#include "../item/ItemIconRenderer.h"

#include "../network/controller/WorldController.h"

namespace lakot
{

class WorldScene final : public Scene
{
public:
    struct InitialSpawnState
    {
        uint32_t mapId;
        float x;
        float y;
        float z;
        float yaw;
    };

    virtual ~WorldScene() override;
    WorldScene(Engine& pEngine, const std::optional<InitialSpawnState>& pSpawnState = std::nullopt, const std::string& pUsername = "Player");

    void setCharacterState(uint64_t pCharacterId, const CharacterStats& pStats, std::vector<OwnedItem> pItems);

    void enter() override;
    void exit() override;
    void update(double pDeltaTime) override;
    void render() override;
    bool handleEvent(SDL_Event* pEvent) override;
    void onSessionResumed() override;

    bool isCapturingMouse() const override;

    bool onCloseRequested() override;

private:
    std::unique_ptr<BoxContainer> mBoxContainer;
    std::unique_ptr<Terrain> mTerrain;
    std::unique_ptr<ThirdPersonCamera> mCamera;
    std::vector<std::shared_ptr<Model>> mMapModels;

    int mViewportWidth{1};
    int mViewportHeight{1};

    std::unique_ptr<BoxContainer> mLocalCharacterBox;

    const MapData* mMap{nullptr};

    std::unique_ptr<ItemAppearance> mItemAppearance;
    std::unique_ptr<ItemIconRenderer> mItemIcons;
    std::unique_ptr<WorldEntities> mEntities;
    std::unique_ptr<EquipmentAttachments> mAttachments;
    std::shared_ptr<Panel> mAttachmentPanel;
    std::unique_ptr<EntityRenderer> mEntityRenderer;
    std::unique_ptr<WorldPicker> mPicker;
    std::unique_ptr<AutoWalk> mAutoWalk;
    CameraBob mCameraBob;
    std::unique_ptr<WorldHud> mHud;

    bool mIsMovingForward{false};
    bool mIsMovingBackward{false};
    bool mIsMovingLeft{false};
    bool mIsMovingRight{false};

    bool mIsOrbiting{false};
    glm::vec2 mRightClickDownPos{0.0f, 0.0f};
    float mOrbitDragDistance{0.0f};

    bool mContextMenuOpenRequested{false};
    glm::vec2 mContextMenuScreenPos{0.0f, 0.0f};
    std::optional<WorldPicker::Target> mContextMenuTarget;

    uint64_t mPendingPickupId{0};

    glm::vec3 mLocalPosition{0.0f, 0.0f, 0.0f};
    glm::vec3 mPreviousLocalPosition{0.0f, 0.0f, 0.0f};

    glm::vec3 mLastSentPosition{0.0f, 0.0f, 0.0f};
    float mLastSentYaw{0.0f};
    bool mHasSentInitialState{false};
    double mStateSendAccumulator{0.0};

    std::optional<InitialSpawnState> mInitialSpawnState;
    std::string mLocalUsername;
    CharacterStats mLocalStats;
    std::vector<OwnedItem> mLocalItems;

    uint64_t mLocalCharacterId{0};
    uint32_t mLocalKingdom{0};
    bool mIsLocalDead{false};
    bool mIsAttackHeld{false};
    bool mHasWarnedSafeZone{false};
    double mAttackCooldown{0.0};
    double mComboResetTimer{0.0};
    double mAttackRootTimer{0.0};
    uint32_t mCombo{0};
    DamageNumbers mDamageNumbers;


    void onWorldSnapshot(const WorldController::WorldSnapshot& pSnapshot);
    void onMapChanged(const WorldController::MapSnapshot& pSnapshot);

    void updateLocalMovement(double pDeltaTime);
    void updatePickup();
    void onPickupResult(services::world::PickupResult pResult);
    glm::vec3 slideLocal(glm::vec3 pPosition, glm::vec2 pStep) const;

    void teleportLocalPosition(const glm::vec3& pPosition);

    void loadMap(uint32_t pMapId);
    void sendLocalPlayerState();

    void endOrbit();

    void renderNameplate(const glm::vec3& pWorldPosition, const std::string& pName, const std::string& pSpeechText,
                         ImU32 pNameColor, int pHealthPercent);
    void renderDamageNumbers(double pExtraSeconds);
    std::optional<ImVec2> projectToScreen(const glm::vec3& pWorldPosition) const;
    ImU32 getNameColor(const WorldEntity& pEntity) const;
    void updateAttack(double pDeltaTime);

    ImVec2 toGlobalImGuiPos(const ImVec2& pWindowLocalPos) const;
};

}

#endif
