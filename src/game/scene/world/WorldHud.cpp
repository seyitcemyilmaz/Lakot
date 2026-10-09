#include "WorldHud.h"

#include "../../network/NetworkManager.h"

using namespace lakot;

WorldHud::WorldHud(RmlUiLayer& pRmlUiLayer,
                   NetworkManager& pNetworkManager,
                   DisplaySettings& pDisplaySettings,
                   SDL_Window* pWindow,
                   const std::string& pUsername,
                   const CharacterStats& pStats,
                   const std::vector<OwnedItem>& pItems,
                   ItemIconRenderer& pItemIcons)
    : mNetworkManager(pNetworkManager)
    , mDisplaySettings(pDisplaySettings)
    , mWindow(pWindow)
    , mMinimap(pRmlUiLayer)
    , mWorldMap(pRmlUiLayer)
    , mWorldChat(pRmlUiLayer)
    , mSkillBar(pRmlUiLayer)
    , mVitals(pRmlUiLayer)
    , mInventory(pRmlUiLayer, pNetworkManager.getInventoryController(), pItemIcons)
    , mCharacterSheet(pRmlUiLayer, pUsername)
    , mSystemMenu(pRmlUiLayer)
    , mWhispers(pRmlUiLayer)
{
    mSystemMenu.setLogoutCallback(
    [this]()
    {
        if (mLogoutCallback)
        {
            mLogoutCallback();
        }
    });

    mSystemMenu.setDisplaySelectCallback(
    [this](size_t pIndex)
    {
        if (pIndex < mDisplayModes.size())
        {
            mDisplaySettings.change(mWindow, mDisplayModes[pIndex]);
            refreshDisplayOptions();
        }
    });

    mWorldChat.setLocalMessageSentCallback(
    [this](bool pIsGlobal, const std::string& pText)
    {
        if (!pIsGlobal && mLocalSayCallback)
        {
            mLocalSayCallback(pText);
        }
    });

    mInventory.setContents(pItems, pStats);
    mCharacterSheet.setStats(pStats);
    mStats = pStats;
    mVitals.setVitals(pStats.health, pStats.maxHealth, pStats.level, pStats.experience, pStats.experienceForNextLevel);

    connectNetwork();
}

void WorldHud::connectNetwork()
{
    ChatController& tChat = mNetworkManager.getChatController();

    tChat.setDirectMessageReceivedCallback(
    [this](uint64_t pFromPlayerId, const std::string& pFromUsername, const std::string& pText)
    {
        mWhispers.onMessageReceived(pFromPlayerId, pFromUsername, pText);
    });

    tChat.setDirectMessageResultCallback(
    [this](uint64_t pRequestId, bool pIsSuccess, const std::string& pMessage)
    {
        mWhispers.onSendResult(pRequestId, pIsSuccess, pMessage);
    });

    tChat.setChatMessageResultCallback(
    [this](bool pIsSuccess, const std::string& pMessage)
    {
        mWorldChat.onSendResult(pIsSuccess, pMessage);
    });

    InventoryController& tInventory = mNetworkManager.getInventoryController();

    tInventory.setInventoryUpdateCallback(
    [this](const std::vector<OwnedItem>& pItems, const CharacterStats& pStats)
    {
        mInventory.setContents(pItems, pStats);
        mCharacterSheet.setStats(pStats);
        mStats = pStats;
        mVitals.setVitals(pStats.health, pStats.maxHealth, pStats.level, pStats.experience, pStats.experienceForNextLevel);

        if (mLocalItemsCallback)
        {
            mLocalItemsCallback(pItems);
        }
    });

    tInventory.setActionResultCallback(
    [this](bool pIsSuccess)
    {
        mInventory.onActionResult(pIsSuccess);
    });

    mInventory.setEquipRequestCallback(
    [this](uint32_t pBagSlot)
    {
        mNetworkManager.getInventoryController().sendEquipItem(pBagSlot);
    });

    mInventory.setUnequipRequestCallback(
    [this](EquipSlotType pSlot, std::optional<uint32_t> pBagSlot)
    {
        mNetworkManager.getInventoryController().sendUnequipItem(pSlot, pBagSlot);
    });

    mInventory.setMoveRequestCallback(
    [this](uint32_t pFromSlot, uint32_t pToSlot)
    {
        mNetworkManager.getInventoryController().sendMoveItem(pFromSlot, pToSlot);
    });
}

void WorldHud::setLogoutCallback(LogoutCallback pCallback)
{
    mLogoutCallback = std::move(pCallback);
}

void WorldHud::setLocalSayCallback(LocalSayCallback pCallback)
{
    mLocalSayCallback = std::move(pCallback);
}

void WorldHud::onSelfVitals(const WorldController::SelfVitalsSnapshot& pVitals)
{
    mStats.health = pVitals.health;
    mStats.maxHealth = pVitals.maxHealth;
    mStats.level = pVitals.level;
    mStats.experience = pVitals.experience;
    mStats.experienceForNextLevel = pVitals.experienceForNextLevel;

    mCharacterSheet.setStats(mStats);
    mVitals.setVitals(pVitals.health, pVitals.maxHealth, pVitals.level, pVitals.experience, pVitals.experienceForNextLevel);
    mVitals.setDeath(pVitals.health <= 0, pVitals.respawnSeconds);
}

void WorldHud::setSafeZone(bool pIsSafe)
{
    mVitals.setSafeZone(pIsSafe);
}

void WorldHud::showNotice(const std::string& pText)
{
    mVitals.showNotice(pText);
}

void WorldHud::setLocalItemsCallback(LocalItemsCallback pCallback)
{
    mLocalItemsCallback = std::move(pCallback);
}

void WorldHud::setMap(const std::string& pImageName, const std::string& pName, float pWorldSize)
{
    mMinimap.setMap(pImageName, pName, pWorldSize);
    mWorldMap.setMap(pImageName, pName, pWorldSize);
}

void WorldHud::update(double pDeltaTime, const glm::vec3& pLocalPosition, float pFacingDegrees, const std::vector<glm::vec3>& pOthers)
{
    mWhispers.update();
    mInventory.update();
    mMinimap.update(pLocalPosition, pFacingDegrees, pOthers);
    mWorldMap.update(pLocalPosition, pFacingDegrees);
    mSkillBar.update(pDeltaTime);
    mVitals.update(pDeltaTime);
    mWorldChat.update(pDeltaTime);
}

bool WorldHud::handleKeyDown(const SDL_KeyboardEvent& pEvent)
{
    if (!pEvent.repeat)
    {
        switch (pEvent.scancode)
        {
            case SDL_SCANCODE_I: mInventory.toggle();      return true;
            case SDL_SCANCODE_M: mWorldMap.toggle();       return true;
            case SDL_SCANCODE_C: mCharacterSheet.toggle(); return true;
            default: break;
        }

        if (pEvent.key == SDLK_ESCAPE)
        {
            handleEscape();
            return true;
        }
    }

    SkillBarSlot tSlot = SkillBarSlot::eCount;

    switch (pEvent.key)
    {
        case SDLK_1:  tSlot = SkillBarSlot::eSlot1;  break;
        case SDLK_2:  tSlot = SkillBarSlot::eSlot2;  break;
        case SDLK_3:  tSlot = SkillBarSlot::eSlot3;  break;
        case SDLK_4:  tSlot = SkillBarSlot::eSlot4;  break;
        case SDLK_5:  tSlot = SkillBarSlot::eSlot5;  break;
        case SDLK_F1: tSlot = SkillBarSlot::eSlotF1; break;
        case SDLK_F2: tSlot = SkillBarSlot::eSlotF2; break;
        case SDLK_F3: tSlot = SkillBarSlot::eSlotF3; break;
        case SDLK_F4: tSlot = SkillBarSlot::eSlotF4; break;
        case SDLK_F5: tSlot = SkillBarSlot::eSlotF5; break;
        default: break;
    }

    if (tSlot != SkillBarSlot::eCount)
    {
        if (!pEvent.repeat)
        {
            mSkillBar.triggerFlash(tSlot);
        }

        return true;
    }

    bool tIsEnterKey = (pEvent.key == SDLK_RETURN || pEvent.key == SDLK_KP_ENTER);

    if (!pEvent.repeat && tIsEnterKey)
    {
        if ((pEvent.mod & SDL_KMOD_SHIFT) != 0)
        {
            mWhispers.openBlank();
        }
        else
        {
            mWorldChat.open();
        }

        return true;
    }

    return false;
}

void WorldHud::handleEscape()
{
    if (mSystemMenu.isOpen())
    {
        if (mSystemMenu.isSettingsOpen())
        {
            mSystemMenu.closeSettings();
        }
        else
        {
            mSystemMenu.close();
        }

        return;
    }

    if (mInventory.isOpen() || mCharacterSheet.isOpen() || mWorldMap.isOpen())
    {
        mInventory.close();
        mCharacterSheet.close();
        mWorldMap.close();
        return;
    }

    if (!mWhispers.minimizeOpen())
    {
        refreshDisplayOptions();
        mSystemMenu.open();
    }
}

bool WorldHud::isSystemMenuOpen() const
{
    return mSystemMenu.isOpen();
}

void WorldHud::closeWorldChat()
{
    mWorldChat.close();
}

void WorldHud::openWhisper(uint64_t pPlayerId, const std::string& pUsername)
{
    mWhispers.open(pPlayerId, pUsername);
}

void WorldHud::onWorldChatMessage(const std::string& pFromUsername, const std::string& pText, bool pIsGlobal)
{
    mWorldChat.appendLine(pFromUsername, pText, false, pIsGlobal);
}

void WorldHud::clearWorldChat()
{
    mWorldChat.clearHistory();
}

void WorldHud::refreshDisplayOptions()
{
    mDisplayModes = DisplaySettings::getAvailableModes(mWindow);

    std::vector<std::string> tLabels;
    size_t tSelected = 0;

    for (size_t tIndex = 0; tIndex < mDisplayModes.size(); ++tIndex)
    {
        tLabels.push_back(mDisplayModes[tIndex].getLabel());

        if (mDisplayModes[tIndex] == mDisplaySettings.getMode())
        {
            tSelected = tIndex;
        }
    }

    mSystemMenu.setDisplayOptions(tLabels, tSelected);
}
