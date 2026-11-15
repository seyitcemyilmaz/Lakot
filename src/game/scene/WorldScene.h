#ifndef LAKOT_WORLDSCENE_H
#define LAKOT_WORLDSCENE_H

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <imgui.h>

#include "Scene.h"

#include "world/AutoWalk.h"
#include "../graphics/camera/CameraBob.h"
#include "world/CombatFeel.h"
#include "world/DamageNumbers.h"
#include "world/EntityRenderer.h"
#include "world/TargetController.h"
#include "world/MapAssetPrefetcher.h"
#include "world/MapModelBatch.h"
#include "world/WorldMapObjects.h"
#include "world/WorldGround.h"
#include "../gui/Panel.h"
#include "../item/EquipmentAttachments.h"
#include "../effect/TrailRenderer.h"
#include "../effect/WeaponTrail.h"
#include "world/WorldEffects.h"
#include "world/WorldEntities.h"
#include "world/WorldHud.h"
#include "world/WorldOverlay.h"
#include "world/WorldPicker.h"

#include "../graphics/camera/ThirdPersonCamera.h"

#include "../graphics/geometry/BoxContainer.h"
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
    std::unique_ptr<WorldMapObjects> mMapObjects;
    std::unique_ptr<WorldGround> mGround;
    std::unique_ptr<ThirdPersonCamera> mCamera;
    std::unique_ptr<MapAssetPrefetcher> mMapPrefetcher;

    int mViewportWidth{1};
    int mViewportHeight{1};

    std::unique_ptr<BoxContainer> mLocalCharacterBox;

    const MapData* mMap{nullptr};

    std::unique_ptr<ItemAppearance> mItemAppearance;
    std::unique_ptr<ItemIconRenderer> mItemIcons;
    std::unique_ptr<WorldEntities> mEntities;
    std::unique_ptr<EquipmentAttachments> mAttachments;
    std::shared_ptr<Panel> mAttachmentPanel;
    std::unique_ptr<WorldEffects> mEffects;
    std::shared_ptr<Panel> mEffectsPanel;
    std::unique_ptr<EntityRenderer> mEntityRenderer;
    std::unique_ptr<WorldPicker> mPicker;
    std::unique_ptr<AutoWalk> mAutoWalk;
    CameraBob mCameraBob;
    CombatFeel mFeel;
    WeaponTrail mWeaponTrail;
    std::unique_ptr<TrailRenderer> mTrailRenderer;
    std::unique_ptr<WorldOverlay> mOverlay;
    std::unique_ptr<WorldHud> mHud;
    std::unique_ptr<TargetController> mTarget;

    bool mIsMovingForward{false};
    bool mIsMovingBackward{false};
    bool mIsMovingLeft{false};
    bool mIsMovingRight{false};

    bool mIsOrbiting{false};
    glm::vec2 mRightClickDownPos{0.0f, 0.0f};
    float mOrbitDragDistance{0.0f};

    uint64_t mPendingPickupId{0};
    uint64_t mPendingShopNpcId{0};
    uint64_t mLastSentPickupId{0};
    std::unordered_map<uint64_t, uint64_t> mSkippedPickups;

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
    void updateShop();
    void requestAutoPickup();
    void onPickupResult(services::world::PickupResult pResult);
    glm::vec3 slideLocal(glm::vec3 pPosition, glm::vec2 pStep) const;

    void teleportLocalPosition(const glm::vec3& pPosition);

    std::string getMapDisplayName() const;
    void loadMap(uint32_t pMapId);
    void sendLocalPlayerState();

    void endOrbit();

    void renderOverlay(double pExtraSeconds);
    std::optional<glm::vec2> projectToScreen(const glm::vec3& pWorldPosition) const;
    WorldOverlay::NameToneType getNameTone(const WorldEntity& pEntity) const;
    void updateAttack(double pDeltaTime);
    void onAnimationEvent(const AnimationEvent& pEvent);
    void sampleTrails();
    bool hasEnemyInFront() const;
    void resetFeel();
};

}

#endif
