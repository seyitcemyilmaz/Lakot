#include "WorldHud.h"

#include "../../item/ItemIconPath.h"
#include "../../item/ItemIconRenderer.h"
#include "../../localization/Localization.h"
#include "../../network/NetworkManager.h"

using namespace lakot;

WorldHud::WorldHud(RmlUiLayer& pRmlUiLayer,
                   NetworkManager& pNetworkManager,
                   const Localization& pLocalization,
                   DisplaySettings& pDisplaySettings,
                   SDL_Window* pWindow,
                   const std::string& pUsername,
                   const CharacterStats& pStats,
                   const ClassCatalog& pClassCatalog,
                   const SkillCatalog& pSkillCatalog,
                   const std::vector<OwnedItem>& pItems,
                   ItemIconRenderer& pItemIcons,
                   uint64_t pCharacterId)
    : mNetworkManager(pNetworkManager)
    , mLocalization(pLocalization)
    , mDisplaySettings(pDisplaySettings)
    , mWindow(pWindow)
    , mItemIcons(pItemIcons)
    , mClassCatalog(pClassCatalog)
    , mBindings(pCharacterId)
    , mMinimap(pRmlUiLayer)
    , mWorldMap(pRmlUiLayer)
    , mWorldChat(pRmlUiLayer, pLocalization)
    , mSkillBar(pRmlUiLayer)
    , mVitals(pRmlUiLayer)
    , mInventory(pRmlUiLayer, pLocalization, pNetworkManager.getInventoryController(), pClassCatalog, pItemIcons)
    , mCharacterSheet(pRmlUiLayer, pLocalization, pClassCatalog, pUsername)
    , mSkillWindow(pRmlUiLayer, pLocalization, pSkillCatalog, pClassCatalog)
    , mSystemMenu(pRmlUiLayer)
    , mItemDiscard(pRmlUiLayer, pLocalization)
    , mShop(pRmlUiLayer, pNetworkManager.getInventoryController(), pLocalization, pItemIcons)
    , mWhispers(pRmlUiLayer, pLocalization)
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

    mWorldChat.setCommandSubmittedCallback(
    [this](const std::string& pText)
    {
        mNetworkManager.getChatController().sendCommand(pText);
    });

    mInventory.setContents(pItems, pStats);
    mShop.setContents(pItems, pStats);
    mCharacterSheet.setStats(pStats);
    mSkillWindow.setStats(pStats);
    mStats = pStats;
    mVitals.setVitals(pStats.health, pStats.maxHealth, pStats.level, pStats.experience, pStats.experienceForNextLevel);
    mVitals.setMana(pStats.mana, pStats.maxMana);

    updateBagCounts(pItems);

    connectNetwork();
    announcePathChoice();
    refreshSkillBar();
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

    tChat.setCommandResultCallback(
    [this](services::chat::CommandStatus pStatus, const std::vector<std::string>& pLines)
    {
        WorldChatRmlController::SystemLineType tType = pStatus == services::chat::COMMAND_OK
            ? WorldChatRmlController::SystemLineType::eInfo
            : WorldChatRmlController::SystemLineType::eError;

        for (const std::string& tLine : pLines)
        {
            mWorldChat.appendSystemLine(tLine, tType);
        }
    });

    tChat.setSystemMessageCallback(
    [this](services::chat::SystemMessageKind pKind, const std::string& pText)
    {
        WorldChatRmlController::SystemLineType tType = WorldChatRmlController::SystemLineType::eInfo;

        switch (pKind)
        {
        case services::chat::SYSTEM_MESSAGE_ANNOUNCE:
            tType = WorldChatRmlController::SystemLineType::eAnnounce;
            break;

        case services::chat::SYSTEM_MESSAGE_ROLL:
            tType = WorldChatRmlController::SystemLineType::eRoll;
            break;

        case services::chat::SYSTEM_MESSAGE_WARNING:
            tType = WorldChatRmlController::SystemLineType::eWarning;
            break;

        default:
            break;
        }

        mWorldChat.appendSystemLine(pText, tType);
    });

    InventoryController& tInventory = mNetworkManager.getInventoryController();

    tInventory.setInventoryUpdateCallback(
    [this](const std::vector<OwnedItem>& pItems, const CharacterStats& pStats)
    {
        mInventory.setContents(pItems, pStats);
        mShop.setContents(pItems, pStats);
        applyStats(pStats);
        mVitals.setVitals(pStats.health, pStats.maxHealth, pStats.level, pStats.experience, pStats.experienceForNextLevel);
        mVitals.setMana(pStats.mana, pStats.maxMana);

        updateBagCounts(pItems);
        refreshSkillBar();

        if (mLocalItemsCallback)
        {
            mLocalItemsCallback(pItems);
        }
    });

    tInventory.setActionResultCallback(
    [this](bool pIsSuccess)
    {
        onInventoryActionResult(pIsSuccess);
    });

    tInventory.setUseItemResultCallback(
    [this](services::inventory::UseItemStatus pStatus, float pCooldownSeconds)
    {
        onUseItemResult(pStatus, pCooldownSeconds);
    });

    mInventory.setItemDroppedOutsideCallback(
    [this](const OwnedItem& pItem, float pX, float pY)
    {
        onItemDroppedOutside(pItem, pX, pY);
    });

    mVitals.setRespawnChoiceCallback(
    [this](services::world::RespawnChoice pChoice)
    {
        mNetworkManager.getWorldController().sendRespawn(pChoice);
    });

    mNetworkManager.getWorldController().setRespawnResultCallback(
    [this](services::world::RespawnStatus pStatus)
    {
        onRespawnResult(pStatus);
    });

    mItemDiscard.setConfirmCallback(
    [this](uint32_t pBagSlot, uint32_t pTemplateId, uint32_t pCount, bool pIsDestroy)
    {
        onDiscardConfirmed(pBagSlot, pTemplateId, pCount, pIsDestroy);
    });

    ShopController& tShop = mNetworkManager.getShopController();

    tShop.setShopOpenedCallback(
    [this](services::shop::ShopStatus pStatus, uint64_t pNpcId, const std::string& pTitle, const std::vector<uint32_t>& pTemplateIds)
    {
        onShopOpened(pStatus, pNpcId, pTitle, pTemplateIds);
    });

    tShop.setBuyResultCallback(
    [this](services::shop::ShopStatus pStatus)
    {
        onShopTradeResult(pStatus);
    });

    tShop.setSellResultCallback(
    [this](services::shop::ShopStatus pStatus)
    {
        onShopTradeResult(pStatus);
    });

    mShop.setBuyCallback(
    [this](uint64_t pNpcId, uint32_t pTemplateId, uint32_t pQuantity)
    {
        mNetworkManager.getShopController().sendBuy(pNpcId, pTemplateId, pQuantity);
    });

    mShop.setSellCallback(
    [this](uint64_t pNpcId, uint32_t pBagSlot, uint32_t pTemplateId, uint32_t pCount)
    {
        mNetworkManager.getShopController().sendSell(pNpcId, pBagSlot, pTemplateId, pCount);
    });

    mSkillBar.setSlotActivatedCallback(
    [this](SkillBarSlot pSlot)
    {
        activateSkillSlot(pSlot);
    });

    mSkillBar.setSlotDroppedCallback(
    [this](SkillBarSlot pFrom, std::optional<SkillBarSlot> pTo)
    {
        onSkillSlotDropped(pFrom, pTo);
    });

    ProgressionController& tProgression = mNetworkManager.getProgressionController();

    tProgression.setAllocateStatResultCallback(
    [this](services::progression::AllocateStatStatus pStatus)
    {
        onAllocateStatResult(pStatus);
    });

    mCharacterSheet.setAllocateStatCallback(
    [this](services::progression::StatType pStat, uint32_t pAmount)
    {
        mNetworkManager.getProgressionController().sendAllocateStat(pStat, pAmount);
    });

    tProgression.setChoosePathResultCallback(
    [this](services::progression::ChoosePathStatus pStatus)
    {
        onChoosePathResult(pStatus);
    });

    mSkillWindow.setChoosePathCallback(
    [this](uint32_t pPathId)
    {
        mNetworkManager.getProgressionController().sendChoosePath(pPathId);
    });

    mSkillWindow.setRaiseSkillCallback(
    [this](uint32_t /*pSkillId*/)
    {
        mWorldChat.appendSystemLine(mLocalization.text("hud.skill_learning_soon"), WorldChatRmlController::SystemLineType::eInfo);
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

    applyStats(mStats);
    mVitals.setVitals(pVitals.health, pVitals.maxHealth, pVitals.level, pVitals.experience, pVitals.experienceForNextLevel);
    mVitals.setDeath(pVitals.health <= 0, pVitals.respawnSeconds, pVitals.autoRespawnSeconds);
}

void WorldHud::setSafeZone(bool pIsSafe)
{
    mVitals.setSafeZone(pIsSafe);
}

void WorldHud::showNotice(const std::string& pText)
{
    mVitals.showNotice(pText);
}

void WorldHud::requestShop(uint64_t pNpcId)
{
    mNetworkManager.getShopController().sendOpenShop(pNpcId);
}

void WorldHud::closeShop()
{
    mShop.close();
}

bool WorldHud::isShopOpen() const
{
    return mShop.isOpen();
}

uint64_t WorldHud::getShopNpcId() const
{
    return mShop.getNpcId();
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
    mShop.update();
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
            case SDL_SCANCODE_K: mSkillWindow.toggle();    return true;
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
            activateSkillSlot(tSlot);
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
    if (mItemDiscard.isOpen())
    {
        mItemDiscard.close();
        return;
    }

    if (mShop.isOpen())
    {
        mShop.close();
        return;
    }

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

    if (mInventory.isOpen() || mCharacterSheet.isOpen() || mSkillWindow.isOpen() || mWorldMap.isOpen())
    {
        mInventory.close();
        mCharacterSheet.close();
        mSkillWindow.close();
        mWorldMap.close();
        return;
    }

    if (!mWhispers.minimizeOpen())
    {
        refreshDisplayOptions();
        mSystemMenu.open();
    }
}

bool WorldHud::isModalOpen() const
{
    return mSystemMenu.isOpen() || mItemDiscard.isOpen();
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
        tLabels.push_back(mDisplayModes[tIndex].getLabel(mLocalization));

        if (mDisplayModes[tIndex] == mDisplaySettings.getMode())
        {
            tSelected = tIndex;
        }
    }

    mSystemMenu.setDisplayOptions(tLabels, tSelected);
}

void WorldHud::updateBagCounts(const std::vector<OwnedItem>& pItems)
{
    mBagCounts.clear();

    for (const OwnedItem& tItem : pItems)
    {
        if (tItem.location == ItemLocationType::eInventory)
        {
            mBagCounts[tItem.templateId] += tItem.count;
        }
    }
}

void WorldHud::refreshSkillBar()
{
    for (size_t tIndex = 0; tIndex < SkillBarBindings::kSlotCount; ++tIndex)
    {
        refreshSkillSlot(tIndex);
    }
}

void WorldHud::refreshSkillSlot(size_t pIndex)
{
    SkillBarSlot tSlot = static_cast<SkillBarSlot>(pIndex);
    std::optional<uint32_t> tTemplateId = mBindings.getTemplateId(pIndex);

    if (!tTemplateId)
    {
        mSkillBar.clearSlot(tSlot);
        return;
    }

    const ItemTemplate* tTemplate = mNetworkManager.getInventoryController().findTemplate(*tTemplateId);
    auto tCount = mBagCounts.find(*tTemplateId);

    mSkillBar.setSlot(tSlot, ItemIconPath::resolve(mItemIcons, tTemplate), tCount == mBagCounts.end() ? 0 : tCount->second);
}

void WorldHud::activateSkillSlot(SkillBarSlot pSlot)
{
    mSkillBar.triggerFlash(pSlot);

    std::optional<uint32_t> tTemplateId = mBindings.getTemplateId(static_cast<size_t>(pSlot));

    if (!tTemplateId || mSkillBar.isCoolingDown(pSlot))
    {
        return;
    }

    mNetworkManager.getInventoryController().sendUseItem(*tTemplateId);
}

void WorldHud::bindSkillSlot(SkillBarSlot pSlot, uint32_t pTemplateId)
{
    mBindings.setTemplateId(static_cast<size_t>(pSlot), pTemplateId);
    refreshSkillSlot(static_cast<size_t>(pSlot));
}

void WorldHud::onSkillSlotDropped(SkillBarSlot pFrom, std::optional<SkillBarSlot> pTo)
{
    std::optional<uint32_t> tTemplateId = mBindings.getTemplateId(static_cast<size_t>(pFrom));

    if (!tTemplateId)
    {
        return;
    }

    mBindings.clear(static_cast<size_t>(pFrom));
    refreshSkillSlot(static_cast<size_t>(pFrom));

    if (pTo)
    {
        bindSkillSlot(*pTo, *tTemplateId);
    }
}

void WorldHud::onItemDroppedOutside(const OwnedItem& pItem, float pX, float pY)
{
    InventoryController& tInventory = mNetworkManager.getInventoryController();
    const ItemTemplate* tTemplate = tInventory.findTemplate(pItem.templateId);

    if (tTemplate && tTemplate->isUsable())
    {
        if (std::optional<SkillBarSlot> tSlot = mSkillBar.getSlotAt(pX, pY))
        {
            bindSkillSlot(*tSlot, pItem.templateId);
            return;
        }
    }

    std::string tName = tTemplate ? mLocalization.content("item." + std::to_string(tTemplate->templateId) + ".name", tTemplate->name) : mLocalization.text("hud.unknown_item");

    mItemDiscard.open(pItem.slot, pItem.templateId, pItem.count, tName, ItemIconPath::resolve(mItemIcons, tTemplate));
}

void WorldHud::onDiscardConfirmed(uint32_t pBagSlot, uint32_t pTemplateId, uint32_t pCount, bool pIsDestroy)
{
    mIsAwaitingDiscardResult = mNetworkManager.getInventoryController().sendDiscardItem(pBagSlot, pTemplateId, pCount, pIsDestroy);
}

void WorldHud::onShopOpened(services::shop::ShopStatus pStatus, uint64_t pNpcId, const std::string& pTitle, const std::vector<uint32_t>& pTemplateIds)
{
    if (pStatus != services::shop::SHOP_STATUS_OK)
    {
        onShopTradeResult(pStatus);
        return;
    }

    mShop.open(pNpcId, pTitle, pTemplateIds);

    if (!mInventory.isOpen())
    {
        mInventory.toggle();
    }
}

void WorldHud::onShopTradeResult(services::shop::ShopStatus pStatus)
{
    std::string tText;

    switch (pStatus)
    {
    case services::shop::SHOP_STATUS_NOT_ENOUGH_GOLD: tText = mLocalization.text("shop.notice.not_enough_gold"); break;
    case services::shop::SHOP_STATUS_BAG_FULL:        tText = mLocalization.text("shop.notice.bag_full"); break;
    case services::shop::SHOP_STATUS_OUT_OF_RANGE:    tText = mLocalization.text("shop.notice.too_far"); break;
    case services::shop::SHOP_STATUS_NOT_SELLABLE:    tText = mLocalization.text("shop.notice.not_sellable"); break;
    case services::shop::SHOP_STATUS_NOT_SOLD_HERE:   tText = mLocalization.text("shop.notice.not_sold_here"); break;
    case services::shop::SHOP_STATUS_GOLD_LIMIT:      tText = mLocalization.text("shop.notice.gold_limit"); break;
    case services::shop::SHOP_STATUS_TOO_FAST:        tText = mLocalization.text("shop.notice.too_fast"); break;
    case services::shop::SHOP_STATUS_DEAD:            tText = mLocalization.text("shop.notice.dead"); break;
    case services::shop::SHOP_STATUS_INVALID:         tText = mLocalization.text("shop.notice.nothing_to_trade"); break;
    default: break;
    }

    if (!tText.empty())
    {
        mVitals.showNotice(tText);
    }
}

void WorldHud::onRespawnResult(services::world::RespawnStatus pStatus)
{
    switch (pStatus)
    {
    case services::world::RESPAWN_STATUS_TOO_EARLY:
        mVitals.showNotice(mLocalization.text("hud.respawn_too_early"));
        break;

    case services::world::RESPAWN_STATUS_INVALID:
        mVitals.showNotice(mLocalization.text("hud.respawn_invalid"));
        break;

    default:
        break;
    }
}

void WorldHud::onInventoryActionResult(bool pIsSuccess)
{
    if (mIsAwaitingDiscardResult)
    {
        mIsAwaitingDiscardResult = false;

        if (!pIsSuccess)
        {
            mVitals.showNotice(mLocalization.text("hud.discard_failed"));
        }

        return;
    }

    mInventory.onActionResult(pIsSuccess);
}

void WorldHud::onAllocateStatResult(services::progression::AllocateStatStatus pStatus)
{
    switch (pStatus)
    {
        case services::progression::ALLOCATE_STAT_NO_POINTS:
            mWorldChat.appendSystemLine(mLocalization.text("hud.stat.no_points"), WorldChatRmlController::SystemLineType::eWarning);
            break;

        case services::progression::ALLOCATE_STAT_AT_CAP:
            mWorldChat.appendSystemLine(mLocalization.text("hud.stat.at_cap"), WorldChatRmlController::SystemLineType::eWarning);
            break;

        case services::progression::ALLOCATE_STAT_INVALID:
            mWorldChat.appendSystemLine(mLocalization.text("hud.stat.invalid"), WorldChatRmlController::SystemLineType::eError);
            break;

        default:
            break;
    }
}

void WorldHud::onUseItemResult(services::inventory::UseItemStatus pStatus, float pCooldownSeconds)
{
    switch (pStatus)
    {
        case services::inventory::USE_ITEM_STATUS_OK:
        case services::inventory::USE_ITEM_STATUS_COOLDOWN:
            for (size_t tIndex = 0; tIndex < SkillBarBindings::kSlotCount; ++tIndex)
            {
                std::optional<uint32_t> tTemplateId = mBindings.getTemplateId(tIndex);
                const ItemTemplate* tTemplate = tTemplateId ? mNetworkManager.getInventoryController().findTemplate(*tTemplateId) : nullptr;

                if (tTemplate && tTemplate->isUsable())
                {
                    mSkillBar.startCooldown(static_cast<SkillBarSlot>(tIndex), pCooldownSeconds);
                }
            }
            break;

        case services::inventory::USE_ITEM_STATUS_DEAD:
            mVitals.showNotice(mLocalization.text("hud.use.dead"));
            break;

        case services::inventory::USE_ITEM_STATUS_FULL:
            mVitals.showNotice(mLocalization.text("hud.use.full"));
            break;

        case services::inventory::USE_ITEM_STATUS_NOT_OWNED:
            mVitals.showNotice(mLocalization.text("hud.use.not_owned"));
            break;

        default:
            break;
    }
}

void WorldHud::applyStats(const CharacterStats& pStats)
{
    mStats = pStats;
    mCharacterSheet.setStats(pStats);
    mSkillWindow.setStats(pStats);
    announcePathChoice();
}

void WorldHud::announcePathChoice()
{
    if (mIsPathHintShown || mStats.pathId != 0 || mStats.classId == 0 || mStats.level < mClassCatalog.getPathUnlockLevel())
    {
        return;
    }

    mIsPathHintShown = true;
    mWorldChat.appendSystemLine(mLocalization.text("hud.path.open"), WorldChatRmlController::SystemLineType::eAnnounce);
}

void WorldHud::onChoosePathResult(services::progression::ChoosePathStatus pStatus)
{
    switch (pStatus)
    {
        case services::progression::CHOOSE_PATH_OK:
            mWorldChat.appendSystemLine(mLocalization.text("hud.path.chosen"), WorldChatRmlController::SystemLineType::eInfo);
            break;

        case services::progression::CHOOSE_PATH_LEVEL_TOO_LOW:
            mWorldChat.appendSystemLine(mLocalization.text("hud.path.level_too_low"), WorldChatRmlController::SystemLineType::eWarning);
            break;

        case services::progression::CHOOSE_PATH_ALREADY_CHOSEN:
            mWorldChat.appendSystemLine(mLocalization.text("hud.path.already_chosen"), WorldChatRmlController::SystemLineType::eWarning);
            break;

        default:
            mWorldChat.appendSystemLine(mLocalization.text("hud.path.invalid"), WorldChatRmlController::SystemLineType::eError);
            break;
    }
}
