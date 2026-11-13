#ifndef LAKOT_SERVER_SHOPCONTROLLER_H
#define LAKOT_SERVER_SHOPCONTROLLER_H

#include <memory>
#include <string>
#include <vector>

#include <connection.pb.h>

#include "BaseController.h"

namespace lakot
{

template <typename MessageType>
class NetworkSession;

class WorldController;
class ItemCatalog;

class ShopController : public BaseController
{
public:
    ShopController(NetworkManager& pNetworkManager,
                   RepositoryManager& pRepositoryManager,
                   WorldController& pWorldController,
                   const ItemCatalog& pItemCatalog);

    void initialize() override;

private:
    WorldController& mWorldController;
    const ItemCatalog& mItemCatalog;

    void handleOpenShopRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleBuyItemRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleSellItemRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);

    static void fillHeader(connection::Message& pMessage, uint64_t pReplyTo, services::shop::ShopStatus pStatus);

    static void sendOpenResult(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                               uint64_t pReplyTo,
                               uint64_t pNpcId,
                               services::shop::ShopStatus pStatus,
                               const std::string& pTitle,
                               const std::vector<uint32_t>& pTemplateIds);

    static void sendBuyResult(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                              uint64_t pReplyTo,
                              uint32_t pTemplateId,
                              uint32_t pQuantity,
                              services::shop::ShopStatus pStatus);

    static void sendSellResult(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                               uint64_t pReplyTo,
                               services::shop::ShopStatus pStatus,
                               uint64_t pGoldGained);
};

}

#endif
