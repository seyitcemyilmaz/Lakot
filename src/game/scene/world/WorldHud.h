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
#include "../../gui/rml/SystemMenuRmlController.h"
#include "../../gui/rml/MinimapRmlController.h"
#include "../../gui/rml/WorldMapRmlController.h"
#include "../../network/GameTypes.h"
#include "../../settings/DisplaySettings.h"
#include "../../settings/SkillBarBindings.h"

namespace lakot
{

class NetworkManager;
class ItemIconRenderer;
class RmlUiLayer;

class WorldHud
{
public:
    using LogoutCallback = std::function<void()>;
    using LocalSayCallback = std::function<void(const std::string& pText)>;
    using LocalItemsCallback = std::function<void(const std::vector<OwnedItem>& pItems)>;

    WorldHud(RmlUiLayer& pRmlUiLayer,
             NetworkManager& pNetworkManager,
             DisplaySettings& pDisplaySettings,
             SDL_Window* pWindow,
             const std::string& pUsername,
             const CharacterStats& pStats,
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

    bool handleKeyDown(const SDL_KeyboardEvent& pEvent);

    void handleEscape();

    bool isSystemMenuOpen() const;

    void closeWorldChat();
    void openWhisper(uint64_t pPlayerId, const std::string& pUsername);

    void onWorldChatMessage(const std::string& pFromUsername, const std::string& pText, bool pIsGlobal);
    void clearWorldChat();

private:
    NetworkManager& mNetworkManager;
    DisplaySettings& mDisplaySettings;
    SDL_Window* mWindow;
    ItemIconRenderer& mItemIcons;
    SkillBarBindings mBindings;

    // Declaration order is document load order, which is draw order.
    MinimapRmlController mMinimap;
    WorldMapRmlController mWorldMap;
    WorldChatRmlController mWorldChat;
    SkillBarRmlController mSkillBar;
    VitalsRmlController mVitals;
    InventoryRmlController mInventory;
    CharacterSheetRmlController mCharacterSheet;
    SystemMenuRmlController mSystemMenu;
    WhisperWindows mWhispers;

    std::vector<DisplayMode> mDisplayModes;

    CharacterStats mStats;
    std::unordered_map<uint32_t, uint32_t> mBagCounts;

    LogoutCallback mLogoutCallback;
    LocalSayCallback mLocalSayCallback;
    LocalItemsCallback mLocalItemsCallback;

    void connectNetwork();
    void refreshDisplayOptions();

    void updateBagCounts(const std::vector<OwnedItem>& pItems);
    void refreshSkillBar();
    void refreshSkillSlot(size_t pIndex);
    void activateSkillSlot(SkillBarSlot pSlot);
    void bindSkillSlot(SkillBarSlot pSlot, uint32_t pTemplateId);
    void onSkillSlotDropped(SkillBarSlot pFrom, std::optional<SkillBarSlot> pTo);
    void onItemDroppedOutside(uint32_t pTemplateId, float pX, float pY);
    void onUseItemResult(services::inventory::UseItemStatus pStatus, float pCooldownSeconds);
};

}

#endif
