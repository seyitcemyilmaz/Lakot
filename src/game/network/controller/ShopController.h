#ifndef LAKOT_SHOPCONTROLLER_H
#define LAKOT_SHOPCONTROLLER_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <connection.pb.h>

namespace lakot
{

class NetworkManager;

template <typename MessageType>
class NetworkSession;

class ShopController
{
public:
    using ShopOpenedCallback = std::function<void(services::shop::ShopStatus pStatus, uint64_t pNpcId, const std::string& pTitle, const std::vector<uint32_t>& pTemplateIds)>;
    using BuyResultCallback = std::function<void(services::shop::ShopStatus pStatus)>;
    using SellResultCallback = std::function<void(services::shop::ShopStatus pStatus)>;

    explicit ShopController(NetworkManager* pNetworkManager);

    void initialize();

    void sendOpenShop(uint64_t pNpcId);
    void sendBuy(uint64_t pNpcId, uint32_t pTemplateId, uint32_t pQuantity);
    void sendSell(uint64_t pNpcId, uint32_t pBagSlot, uint32_t pTemplateId, uint32_t pCount);

    void setShopOpenedCallback(ShopOpenedCallback pCallback);
    void setBuyResultCallback(BuyResultCallback pCallback);
    void setSellResultCallback(SellResultCallback pCallback);

    void clearCallbacks();

private:
    NetworkManager* mNetworkManager;

    ShopOpenedCallback mShopOpenedCallback;
    BuyResultCallback mBuyResultCallback;
    SellResultCallback mSellResultCallback;

    void handleOpenShopResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleBuyItemResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleSellItemResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
};

}

#endif
