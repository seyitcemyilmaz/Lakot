#include "ShopController.h"

#include "../NetworkManager.h"

using namespace lakot;

ShopController::ShopController(NetworkManager* pNetworkManager)
    : mNetworkManager(pNetworkManager)
{

}

void ShopController::initialize()
{
    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kOpenShopResponse, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleOpenShopResponse(pSession, pMessage);
        }
    );

    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kBuyItemResponse, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleBuyItemResponse(pSession, pMessage);
        }
    );

    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kSellItemResponse, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleSellItemResponse(pSession, pMessage);
        }
    );
}

void ShopController::sendOpenShop(uint64_t pNpcId)
{
    connection::Message tMessage;
    tMessage.mutable_request()->mutable_open_shop_request()->set_npc_id(pNpcId);

    mNetworkManager->sendRequest(tMessage);
}

void ShopController::sendBuy(uint64_t pNpcId, uint32_t pTemplateId, uint32_t pQuantity)
{
    connection::Message tMessage;
    auto* tRequest = tMessage.mutable_request()->mutable_buy_item_request();
    tRequest->set_npc_id(pNpcId);
    tRequest->set_item_id(pTemplateId);
    tRequest->set_quantity(pQuantity);

    mNetworkManager->sendRequest(tMessage);
}

void ShopController::sendSell(uint64_t pNpcId, uint32_t pBagSlot, uint32_t pTemplateId, uint32_t pCount)
{
    connection::Message tMessage;
    auto* tRequest = tMessage.mutable_request()->mutable_sell_item_request();
    tRequest->set_npc_id(pNpcId);
    tRequest->set_bag_slot(pBagSlot);
    tRequest->set_template_id(pTemplateId);
    tRequest->set_count(pCount);

    mNetworkManager->sendRequest(tMessage);
}

void ShopController::setShopOpenedCallback(ShopOpenedCallback pCallback)
{
    mShopOpenedCallback = std::move(pCallback);
}

void ShopController::setBuyResultCallback(BuyResultCallback pCallback)
{
    mBuyResultCallback = std::move(pCallback);
}

void ShopController::setSellResultCallback(SellResultCallback pCallback)
{
    mSellResultCallback = std::move(pCallback);
}

void ShopController::clearCallbacks()
{
    mShopOpenedCallback = nullptr;
    mBuyResultCallback = nullptr;
    mSellResultCallback = nullptr;
}

void ShopController::handleOpenShopResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mShopOpenedCallback)
    {
        return;
    }

    const auto& tResponse = pMessage.response().open_shop_response();
    std::vector<uint32_t> tIds(tResponse.template_ids().begin(), tResponse.template_ids().end());

    mShopOpenedCallback(tResponse.status(), tResponse.npc_id(), tResponse.title(), tIds);
}

void ShopController::handleBuyItemResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (mBuyResultCallback)
    {
        mBuyResultCallback(pMessage.response().buy_item_response().status());
    }
}

void ShopController::handleSellItemResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (mSellResultCallback)
    {
        mSellResultCallback(pMessage.response().sell_item_response().status());
    }
}
