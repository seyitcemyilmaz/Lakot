#ifndef LAKOT_WORLD_HUD_H
#define LAKOT_WORLD_HUD_H

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "WhisperWindows.h"

#include "../../gui/rml/WorldChatRmlController.h"
#include "../../gui/rml/SkillBarRmlController.h"
#include "../../gui/rml/VitalsRmlController.h"
#include "../../network/controller/WorldController.h"
#include "../../gui/rml/InventoryRmlController.h"
#include "../../gui/rml/CharacterSheetRmlController.h"
#include "../../gui/rml/SkillWindowRmlController.h"
#include "../../gui/rml/SystemMenuRmlController.h"
#include "../../gui/rml/SettingsRmlController.h"
#include "../../gui/rml/ItemDiscardRmlController.h"
#include "../../gui/rml/ShopRmlController.h"
#include "../../gui/rml/MinimapRmlController.h"
#include "../../gui/rml/WorldMapRmlController.h"
#include "../../network/GameTypes.h"
#include "../../settings/DisplaySettings.h"
#include "../../settings/KeyBindings.h"
#include "../../settings/SkillBarBindings.h"

namespace lakot
{

class NetworkManager;
class ItemIconRenderer;
class RmlUiLayer;
class Localization;

class WorldHud
{
public:
    using LogoutCallback = std::function<void()>;
    using LocalSayCallback = std::function<void(const std::string& pText)>;
    using LocalItemsCallback = std::function<void(const std::vector<OwnedItem>& pItems)>;

    WorldHud(RmlUiLayer& pRmlUiLayer,
             NetworkManager& pNetworkManager,
             const Localization& pLocalization,
             DisplaySettings& pDisplaySettings,
             KeyBindings& pKeyBindings,
             SDL_Window* pWindow,
             const std::string& pUsername,
             const CharacterStats& pStats,
             const ClassCatalog& pClassCatalog,
             const SkillCatalog& pSkillCatalog,
             const std::vector<OwnedItem>& pItems,
             ItemIconRenderer& pItemIcons,
             uint64_t pCharacterId);

    void setLogoutCallback(LogoutCallback pCallback);

    void setLocalSayCallback(LocalSayCallback pCallback);

    void setLocalItemsCallback(LocalItemsCallback pCallback);

    void setMap(const std::string& pImageName, const std::string& pName, float pWorldSize);

    void update(double pDeltaTime, const glm::vec3& pLocalPosition, float pFacingDegrees, const std::vector<glm::vec3>& pOthers);

    void onSelfVitals(const WorldController::SelfVitalsSnapshot& pVitals);
    void setSafeZone(bool pIsSafe);
    void showNotice(const std::string& pText);

    void requestShop(uint64_t pNpcId);
    void closeShop();
    bool isShopOpen() const;
    uint64_t getShopNpcId() const;

    bool handleKeyDown(const SDL_KeyboardEvent& pEvent);
    bool isCapturingKey() const;

    void handleEscape();

    bool isModalOpen() const;

    void closeWorldChat();
    void openWhisper(uint64_t pPlayerId, const std::string& pUsername);

    void onWorldChatMessage(const std::string& pFromUsername, const std::string& pText, bool pIsGlobal);
    void clearWorldChat();

private:
    NetworkManager& mNetworkManager;
    const Localization& mLocalization;
    KeyBindings& mKeyBindings;
    ItemIconRenderer& mItemIcons;
    const ClassCatalog& mClassCatalog;
    SkillBarBindings mBindings;

    // Declaration order is document load order, which is draw order.
    MinimapRmlController mMinimap;
    WorldMapRmlController mWorldMap;
    WorldChatRmlController mWorldChat;
    SkillBarRmlController mSkillBar;
    VitalsRmlController mVitals;
    InventoryRmlController mInventory;
    CharacterSheetRmlController mCharacterSheet;
    SkillWindowRmlController mSkillWindow;
    SystemMenuRmlController mSystemMenu;
    SettingsRmlController mSettings;
    ItemDiscardRmlController mItemDiscard;
    ShopRmlController mShop;
    WhisperWindows mWhispers;

    CharacterStats mStats;
    std::unordered_map<uint32_t, uint32_t> mBagCounts;

    LogoutCallback mLogoutCallback;
    LocalSayCallback mLocalSayCallback;
    LocalItemsCallback mLocalItemsCallback;

    bool mIsAwaitingDiscardResult{false};
    bool mIsPathHintShown{false};

    void connectNetwork();

    void updateBagCounts(const std::vector<OwnedItem>& pItems);
    void refreshSkillBar();
    void refreshSkillSlot(size_t pIndex);
    void activateSkillSlot(SkillBarSlot pSlot);
    void bindSkillSlot(SkillBarSlot pSlot, uint32_t pTemplateId);
    void onSkillSlotDropped(SkillBarSlot pFrom, std::optional<SkillBarSlot> pTo);
    void onItemDroppedOutside(const OwnedItem& pItem, float pX, float pY);
    void onDiscardConfirmed(uint32_t pBagSlot, uint32_t pTemplateId, uint32_t pCount, bool pIsDestroy);
    void onInventoryActionResult(bool pIsSuccess);
    void onShopOpened(services::shop::ShopStatus pStatus, uint64_t pNpcId, const std::string& pTitle, const std::vector<uint32_t>& pTemplateIds);
    void onShopTradeResult(services::shop::ShopStatus pStatus);
    void onRespawnResult(services::world::RespawnStatus pStatus);
    void onUseItemResult(services::inventory::UseItemStatus pStatus, float pCooldownSeconds);
    void onAllocateStatResult(services::progression::AllocateStatStatus pStatus);
    void onChoosePathResult(services::progression::ChoosePathStatus pStatus);
    void applyStats(const CharacterStats& pStats);
    void announcePathChoice();
};

}

#endif
