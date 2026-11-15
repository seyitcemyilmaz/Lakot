#include "ShopController.h"

#include "WorldController.h"

#include "../../game/ItemCatalog.h"

using namespace lakot;

ShopController::ShopController(NetworkManager& pNetworkManager,
                               RepositoryManager& pRepositoryManager,
                               WorldController& pWorldController,
                               const ItemCatalog& pItemCatalog)
    : BaseController(pNetworkManager, pRepositoryManager)
    , mWorldController(pWorldController)
    , mItemCatalog(pItemCatalog)
{

}

void ShopController::initialize()
{
    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kOpenShopRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleOpenShopRequest(pSession, pMessage);
        }
    );

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kBuyItemRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleBuyItemRequest(pSession, pMessage);
        }
    );

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kSellItemRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleSellItemRequest(pSession, pMessage);
        }
    );
}

void ShopController::sendOpenResult(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                                    uint64_t pReplyTo,
                                    uint64_t pNpcId,
                                    services::shop::ShopStatus pStatus,
                                    const std::string& pTitle,
                                    const std::vector<uint32_t>& pTemplateIds)
{
    connection::Message tResponse = makeResponse(pReplyTo, pStatus == services::shop::SHOP_STATUS_OK ? common::STATUS_OK : common::STATUS_INVALID_REQUEST);

    auto* tOut = tResponse.mutable_response()->mutable_open_shop_response();
    tOut->set_status(pStatus);
    tOut->set_npc_id(pNpcId);
    tOut->set_title(pTitle);

    for (uint32_t tTemplateId : pTemplateIds)
    {
        tOut->add_template_ids(tTemplateId);
    }

    pSession->send(tResponse);
}

void ShopController::sendBuyResult(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                                   uint64_t pReplyTo,
                                   uint32_t pTemplateId,
                                   uint32_t pQuantity,
                                   services::shop::ShopStatus pStatus)
{
    connection::Message tResponse = makeResponse(pReplyTo, pStatus == services::shop::SHOP_STATUS_OK ? common::STATUS_OK : common::STATUS_INVALID_REQUEST);

    auto* tOut = tResponse.mutable_response()->mutable_buy_item_response();
    tOut->set_status(pStatus);
    tOut->set_item_id(pTemplateId);
    tOut->set_quantity(pQuantity);

    pSession->send(tResponse);
}

void ShopController::sendSellResult(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                                    uint64_t pReplyTo,
                                    services::shop::ShopStatus pStatus,
                                    uint64_t pGoldGained)
{
    connection::Message tResponse = makeResponse(pReplyTo, pStatus == services::shop::SHOP_STATUS_OK ? common::STATUS_OK : common::STATUS_INVALID_REQUEST);

    auto* tOut = tResponse.mutable_response()->mutable_sell_item_response();
    tOut->set_status(pStatus);
    tOut->set_gold_gained(pGoldGained);

    pSession->send(tResponse);
}

void ShopController::handleOpenShopRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tCharacterId = pSession->getCharacterId();

    if (tCharacterId == 0)
    {
        return;
    }

    const auto& tRequest = pMessage.request().open_shop_request();
    uint64_t tReplyTo = pMessage.request().header().id();
    uint64_t tNpcId = tRequest.npc_id();

    if (tNpcId == 0)
    {
        sendOpenResult(pSession, tReplyTo, tNpcId, services::shop::SHOP_STATUS_INVALID, std::string(), {});
        return;
    }

    mWorldController.openShop(tCharacterId, tNpcId,
    [pSession, tReplyTo, tNpcId](const ShopOpenResult& pResult)
    {
        sendOpenResult(pSession, tReplyTo, tNpcId, pResult.status, pResult.title, pResult.templateIds);
    });
}

void ShopController::handleBuyItemRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tCharacterId = pSession->getCharacterId();

    if (tCharacterId == 0)
    {
        return;
    }

    const auto& tRequest = pMessage.request().buy_item_request();
    uint64_t tReplyTo = pMessage.request().header().id();
    uint32_t tTemplateId = tRequest.item_id();
    uint32_t tQuantity = tRequest.quantity();

    const ItemTemplate* tTemplate = mItemCatalog.find(tTemplateId);

    if (tRequest.npc_id() == 0 || !tTemplate || tQuantity == 0 || tQuantity > tTemplate->maxStack)
    {
        sendBuyResult(pSession, tReplyTo, tTemplateId, tQuantity, services::shop::SHOP_STATUS_INVALID);
        return;
    }

    mWorldController.buyFromShop(tCharacterId, tRequest.npc_id(), tTemplateId, tQuantity,
    [pSession, tReplyTo, tTemplateId, tQuantity](services::shop::ShopStatus pStatus, uint64_t)
    {
        sendBuyResult(pSession, tReplyTo, tTemplateId, tQuantity, pStatus);
    });
}

void ShopController::handleSellItemRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tCharacterId = pSession->getCharacterId();

    if (tCharacterId == 0)
    {
        return;
    }

    const auto& tRequest = pMessage.request().sell_item_request();
    uint64_t tReplyTo = pMessage.request().header().id();

    const ItemTemplate* tTemplate = mItemCatalog.find(tRequest.template_id());

    if (tRequest.npc_id() == 0 || !tTemplate || tRequest.count() > tTemplate->maxStack)
    {
        sendSellResult(pSession, tReplyTo, services::shop::SHOP_STATUS_INVALID, 0);
        return;
    }

    mWorldController.sellToShop(tCharacterId, tRequest.npc_id(), tRequest.bag_slot(), tRequest.template_id(), tRequest.count(),
    [pSession, tReplyTo](services::shop::ShopStatus pStatus, uint64_t pGold)
    {
        sendSellResult(pSession, tReplyTo, pStatus, pGold);
    });
}
