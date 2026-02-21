#include "NetworkManager.h"

using namespace lakot;

NetworkManager::~NetworkManager()
{
    stop();
}

NetworkManager::NetworkManager()
    : mNetworkServer(12345)
    , mDispatcher()
    , mSessionRegistry()
{
    mDispatcher.setUnknownPacketHandler(
    [](std::shared_ptr<NetworkSession<connection::Message>> pSession, int pCaseId)
    {
        std::cerr << "[Security] Bilinmeyen Paket ID: " << pCaseId << " IP: " << pSession->getSocket().remote_endpoint() << std::endl;
    });

    mNetworkServer.setOnClientConnect(
    [this](std::shared_ptr<NetworkSession<connection::Message>> pSession)
    {
        std::cout << "[Network] Yeni Baglanti: " << pSession->getSocket().remote_endpoint() << std::endl;

        pSession->setOnErrorFunction(
        [this](std::shared_ptr<NetworkSession<connection::Message>> pS, const boost::system::error_code& pEc)
        {
            uint64_t tUserId = pS->getUserId();

            if (tUserId != 0)
            {
                std::cout << "[Network] Kullanici Dustu: " << tUserId << std::endl;
                mSessionRegistry.remove(tUserId, pS);
            }
        });

        pSession->setOnMessageFunction(
        [this](std::shared_ptr<NetworkSession<connection::Message>> pS, const connection::Message& pMsg)
        {
            mDispatcher.dispatch(pS, pMsg,
            [](const connection::Message& m) -> uint32_t
            {
                if (m.has_request() && m.request().payload_case() != 0)
                {
                    return MessageDispatcher<connection::Message>::generateKey(
                        m.request().payload_case(),
                        PacketType::Request
                    );
                }

                if (m.has_response() && m.response().payload_case() != 0)
                {
                    return MessageDispatcher<connection::Message>::generateKey(
                        m.response().payload_case(),
                        PacketType::Response
                    );
                }

                return 0;
            });
        });
    });
}

void NetworkManager::start()
{
    mNetworkServer.start(4);
}

void NetworkManager::stop()
{
    mNetworkServer.stop();
}

NetworkServer<connection::Message>& NetworkManager::getNetworkServer()
{
    return mNetworkServer;
}

MessageDispatcher<connection::Message>& NetworkManager::getDispatcher()
{
    return mDispatcher;
}

SessionRegistry<NetworkSession<connection::Message>>& NetworkManager::getSessionRegistry()
{
    return mSessionRegistry;
}
