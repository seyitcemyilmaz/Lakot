#include "InventoryController.h"

#include "WorldController.h"

#include "../../game/ItemCatalog.h"
#include "../../game/Inventory.h"

using namespace lakot;

InventoryController::InventoryController(NetworkManager& pNetworkManager,
                                         RepositoryManager& pRepositoryManager,
                                         WorldController& pWorldController,
                                         const ItemCatalog& pItemCatalog)
    : BaseController(pNetworkManager, pRepositoryManager)
    , mWorldController(pWorldController)
    , mItemCatalog(pItemCatalog)
{

}

void InventoryController::initialize()
{
    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kEquipItemRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleEquipItemRequest(pSession, pMessage);
        }
    );

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kUnequipItemRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleUnequipItemRequest(pSession, pMessage);
        }
    );

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kMoveItemRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleMoveItemRequest(pSession, pMessage);
        }
    );
}

void InventoryController::sendItemTemplates(const std::shared_ptr<NetworkSession<connection::Message>>& pSession)
{
    if (!pSession)
    {
        return;
    }

    connection::Message tMessage;
    auto* tList = tMessage.mutable_response()->mutable_item_template_list();

    tList->set_grid_width(Inventory::kGridWidth);
    tList->set_page_height(Inventory::kPageHeight);
    tList->set_page_count(Inventory::kPageCount);

    for (const ItemTemplate& tTemplate : mItemCatalog.getAll())
    {
        auto* tOut = tList->add_templates();
        tOut->set_template_id(tTemplate.templateId);
        tOut->set_name(tTemplate.name);
        tOut->set_type(static_cast<uint32_t>(tTemplate.type));
        tOut->set_equip_slot(static_cast<uint32_t>(tTemplate.equipSlot));
        tOut->set_max_stack(tTemplate.maxStack);
        tOut->set_required_level(tTemplate.requiredLevel);
        tOut->set_bonus_attack(tTemplate.bonusAttack);
        tOut->set_bonus_defense(tTemplate.bonusDefense);
        tOut->set_bonus_max_health(tTemplate.bonusMaxHealth);
        tOut->set_bonus_max_mana(tTemplate.bonusMaxMana);
        tOut->set_width(tTemplate.width);
        tOut->set_height(tTemplate.height);
        tOut->set_icon(tTemplate.icon);
        tOut->set_model(tTemplate.model);
        tOut->set_tint(tTemplate.tint);
    }

    pSession->send(tMessage);
}

void InventoryController::sendActionResult(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                                           uint64_t pReplyTo,
                                           bool pIsSuccess)
{
    connection::Message tResponse;

    auto* tHeader = tResponse.mutable_response()->mutable_header();
    tHeader->set_reply_to(pReplyTo);
    tHeader->mutable_status()->set_code(pIsSuccess ? common::STATUS_OK : common::STATUS_INVALID_REQUEST);

    tResponse.mutable_response()->mutable_inventory_action_response()->set_status(
        pIsSuccess ? services::inventory::INVENTORY_ACTION_OK
                   : services::inventory::INVENTORY_ACTION_FAILED);

    pSession->send(tResponse);
}

void InventoryController::handleEquipItemRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tCharacterId = pSession->getCharacterId();

    if (tCharacterId == 0)
    {
        return; // not in the world
    }

    uint32_t tBagSlot = pMessage.request().equip_item_request().bag_slot();
    uint64_t tReplyTo = pMessage.request().header().id();

    // The decision is made on the zone thread, where the character actually
    // lives - nothing here reads or writes inventory state directly. The
    // captured session keeps itself alive until the answer is sent.
    mWorldController.withCharacter(tCharacterId,
    [pSession, tReplyTo, tBagSlot, this](Character& pCharacter) -> bool
    {
        bool tIsSuccess = pCharacter.equipItem(tBagSlot);

        sendActionResult(pSession, tReplyTo, tIsSuccess);

        // Returning true makes the zone push the resulting InventoryUpdate;
        // a refused equip changed nothing, so there is nothing to push.
        return tIsSuccess;
    });
}

void InventoryController::handleUnequipItemRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tCharacterId = pSession->getCharacterId();

    if (tCharacterId == 0)
    {
        return;
    }

    const auto& tRequest = pMessage.request().unequip_item_request();
    uint32_t tEquipSlot = tRequest.equip_slot();
    std::optional<uint32_t> tBagSlot;

    if (tRequest.has_bag_slot())
    {
        tBagSlot = tRequest.bag_slot();
    }

    uint64_t tReplyTo = pMessage.request().header().id();

    if (tEquipSlot == 0 || tEquipSlot >= static_cast<uint32_t>(EquipSlotType::eCount))
    {
        sendActionResult(pSession, tReplyTo, false);
        return;
    }

    mWorldController.withCharacter(tCharacterId,
    [pSession, tReplyTo, tEquipSlot, tBagSlot, this](Character& pCharacter) -> bool
    {
        bool tIsSuccess = pCharacter.unequipItem(static_cast<EquipSlotType>(tEquipSlot), tBagSlot);

        sendActionResult(pSession, tReplyTo, tIsSuccess);

        return tIsSuccess;
    });
}

void InventoryController::handleMoveItemRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tCharacterId = pSession->getCharacterId();

    if (tCharacterId == 0)
    {
        return;
    }

    uint32_t tFromSlot = pMessage.request().move_item_request().from_slot();
    uint32_t tToSlot = pMessage.request().move_item_request().to_slot();
    uint64_t tReplyTo = pMessage.request().header().id();

    mWorldController.withCharacter(tCharacterId,
    [pSession, tReplyTo, tFromSlot, tToSlot, this](Character& pCharacter) -> bool
    {
        bool tIsSuccess = pCharacter.moveItem(tFromSlot, tToSlot);

        sendActionResult(pSession, tReplyTo, tIsSuccess);

        return tIsSuccess;
    });
}
