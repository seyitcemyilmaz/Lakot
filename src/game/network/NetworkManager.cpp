#include "NetworkManager.h"

using namespace lakot;

NetworkManager::NetworkManager()
    : mClient()
    , mDispatcher()
    , mAuthController(this)
{
    mAuthController.initialize();

    mClient.setOnMessageReceived(
    [this](const connection::Message& pMessage)
    {
        auto tSession = mClient.getSession();

        if (tSession)
        {
            mDispatcher.dispatch(tSession, pMessage,
            [](const connection::Message& m) -> uint32_t
            {
                if (m.has_request() && m.request().payload_case() != 0)
                {
                    return lakot::MessageDispatcher<connection::Message>::generateKey(
                        m.request().payload_case(),
                        lakot::PacketType::Request
                    );
                }

                if (m.has_response() && m.response().payload_case() != 0)
                {
                    return lakot::MessageDispatcher<connection::Message>::generateKey(
                        m.response().payload_case(),
                        lakot::PacketType::Response
                    );
                }
                return 0;
            });
        }
        else
        {
            std::cout << "[NetworkManager] Mesaj islenemedi: Session yok." << std::endl;
        }
    });

    mClient.setOnConnectionStatusChanged(
    [this](bool pIsConnected)
    {
        if (pIsConnected)
        {
            mClient.getSession()->setRequestIdSetter(
            [](connection::Message& pMsg, uint32_t pId)
            {
                if (pMsg.has_request())
                {
                    pMsg.mutable_request()->mutable_header()->set_request_id(pId);
                }
            });
        }
        else
        {
            std::cout << "Baglanti Koptu / Tekrar deneniyor..." << std::endl;
        }
    });
}

void NetworkManager::start()
{
    mClient.connect("127.0.0.1", 12345);
}

void NetworkManager::update()
{
    mClient.update();
}

void NetworkManager::stop()
{
    mClient.stop();
}

NetworkClient<connection::Message>& NetworkManager::getClient()
{
    return mClient;
}

MessageDispatcher<connection::Message>& NetworkManager::getDispatcher()
{
    return mDispatcher;
}

AuthController& NetworkManager::getAuthController()
{
    return mAuthController;
}
