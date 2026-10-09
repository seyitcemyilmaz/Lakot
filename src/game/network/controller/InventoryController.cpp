#include "InventoryController.h"

#include "../NetworkManager.h"

using namespace lakot;

InventoryController::InventoryController(NetworkManager* pNetworkManager)
    : mNetworkManager(pNetworkManager)
{

}

void InventoryController::initialize()
{
    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kItemTemplateList, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleItemTemplateList(pSession, pMessage);
        }
    );

    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kInventoryUpdate, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleInventoryUpdate(pSession, pMessage);
        }
    );

    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kInventoryActionResponse, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleActionResponse(pSession, pMessage);
        }
    );
}

void InventoryController::sendEquipItem(uint32_t pBagSlot)
{
    auto tSession = mNetworkManager->getClient().getSession();

    if (!tSession)
    {
        return;
    }

    connection::Message tMessage;
    tMessage.mutable_request()->mutable_header()->set_id(mNetworkManager->nextRequestId());
    tMessage.mutable_request()->mutable_equip_item_request()->set_bag_slot(pBagSlot);

    tSession->send(tMessage);
}

void InventoryController::sendUnequipItem(EquipSlotType pEquipSlot, std::optional<uint32_t> pBagSlot)
{
    auto tSession = mNetworkManager->getClient().getSession();

    if (!tSession)
    {
        return;
    }

    connection::Message tMessage;
    tMessage.mutable_request()->mutable_header()->set_id(mNetworkManager->nextRequestId());
    auto* tRequest = tMessage.mutable_request()->mutable_unequip_item_request();
    tRequest->set_equip_slot(static_cast<uint32_t>(pEquipSlot));

    if (pBagSlot)
    {
        tRequest->set_bag_slot(*pBagSlot);
    }

    tSession->send(tMessage);
}

void InventoryController::sendMoveItem(uint32_t pFromSlot, uint32_t pToSlot)
{
    auto tSession = mNetworkManager->getClient().getSession();

    if (!tSession)
    {
        return;
    }

    connection::Message tMessage;
    tMessage.mutable_request()->mutable_header()->set_id(mNetworkManager->nextRequestId());
    tMessage.mutable_request()->mutable_move_item_request()->set_from_slot(pFromSlot);
    tMessage.mutable_request()->mutable_move_item_request()->set_to_slot(pToSlot);

    tSession->send(tMessage);
}

void InventoryController::setInventoryUpdateCallback(InventoryUpdateCallback pCallback)
{
    mInventoryUpdateCallback = pCallback;
}

void InventoryController::setActionResultCallback(ActionResultCallback pCallback)
{
    mActionResultCallback = pCallback;
}

void InventoryController::clearCallbacks()
{
    mInventoryUpdateCallback = nullptr;
    mActionResultCallback = nullptr;

    // mTemplates is deliberately NOT cleared: the catalog belongs to the
    // connection, not to whichever scene happens to be up, and is only resent
    // when a character next enters the world.
}

const ItemTemplate* InventoryController::findTemplate(uint32_t pTemplateId) const
{
    auto tIterator = mTemplates.find(pTemplateId);
    return tIterator == mTemplates.end() ? nullptr : &tIterator->second;
}

bool InventoryController::hasTemplates() const
{
    return !mTemplates.empty();
}

const InventoryGrid& InventoryController::getGrid() const
{
    return mGrid;
}

void InventoryController::handleItemTemplateList(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    const auto& tList = pMessage.response().item_template_list();

    // Guarded: a server that sends zeros (an older build, a partial message)
    // would otherwise leave the client drawing a zero-sized bag.
    if (tList.grid_width() > 0 && tList.page_height() > 0 && tList.page_count() > 0)
    {
        mGrid.width = tList.grid_width();
        mGrid.pageHeight = tList.page_height();
        mGrid.pageCount = tList.page_count();
    }

    mTemplates.clear();
    mTemplates.reserve(tList.templates_size());

    for (const auto& tIncoming : tList.templates())
    {
        ItemTemplate tTemplate;
        tTemplate.templateId = tIncoming.template_id();
        tTemplate.name = tIncoming.name();
        tTemplate.type = tIncoming.type();
        tTemplate.equipSlot = static_cast<EquipSlotType>(tIncoming.equip_slot());
        tTemplate.maxStack = tIncoming.max_stack();
        tTemplate.requiredLevel = tIncoming.required_level();
        tTemplate.bonusAttack = tIncoming.bonus_attack();
        tTemplate.bonusDefense = tIncoming.bonus_defense();
        tTemplate.bonusMaxHealth = tIncoming.bonus_max_health();
        tTemplate.bonusMaxMana = tIncoming.bonus_max_mana();
        tTemplate.width = tIncoming.width() > 0 ? tIncoming.width() : 1;
        tTemplate.height = tIncoming.height() > 0 ? tIncoming.height() : 1;
        tTemplate.icon = tIncoming.icon();
        tTemplate.model = tIncoming.model();
        tTemplate.tint = tIncoming.tint();

        mTemplates.emplace(tTemplate.templateId, std::move(tTemplate));
    }
}

void InventoryController::handleInventoryUpdate(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mInventoryUpdateCallback)
    {
        return;
    }

    const auto& tUpdate = pMessage.response().inventory_update();

    std::vector<OwnedItem> tItems;
    tItems.reserve(tUpdate.items_size());

    for (const auto& tItem : tUpdate.items())
    {
        tItems.push_back({ tItem.template_id(), tItem.count(),
                           static_cast<ItemLocationType>(tItem.location()), tItem.slot() });
    }

    const auto& tIncomingStats = tUpdate.stats();

    CharacterStats tStats;
    tStats.level = tIncomingStats.level();
    tStats.experience = tIncomingStats.experience();
    tStats.experienceForNextLevel = tIncomingStats.experience_for_next_level();
    tStats.health = tIncomingStats.health();
    tStats.maxHealth = tIncomingStats.max_health();
    tStats.mana = tIncomingStats.mana();
    tStats.maxMana = tIncomingStats.max_mana();
    tStats.strength = tIncomingStats.strength();
    tStats.dexterity = tIncomingStats.dexterity();
    tStats.intelligence = tIncomingStats.intelligence();
    tStats.vitality = tIncomingStats.vitality();
    tStats.attack = tIncomingStats.attack();
    tStats.defense = tIncomingStats.defense();
    tStats.gold = tIncomingStats.gold();

    mInventoryUpdateCallback(tItems, tStats);
}

void InventoryController::handleActionResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mActionResultCallback)
    {
        return;
    }

    bool tIsSuccess = pMessage.response().inventory_action_response().status()
                      == services::inventory::INVENTORY_ACTION_OK;

    mActionResultCallback(tIsSuccess);
}
