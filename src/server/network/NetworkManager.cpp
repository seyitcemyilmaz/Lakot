#include "NetworkManager.h"

#include <iostream>
#include <syncstream>

using namespace lakot;

NetworkManager::~NetworkManager()
{
    stop();
}

std::string NetworkManager::getRemoteAddress(const std::shared_ptr<NetworkSession<connection::Message>>& pSession)
{
    boost::system::error_code tErrorCode;
    auto tEndpoint = pSession->getSocket().remote_endpoint(tErrorCode);

    if (tErrorCode)
    {
        return {};
    }

    return tEndpoint.address().to_string();
}

NetworkManager::NetworkManager()
    : mNetworkServer(12345)
    , mDispatcher()
    , mSessionRegistry()
    , mSessionResumeManager(mNetworkServer.getIOContext())
{
    mSessionResumeManager.setReleaseCallback(
    [this](uint64_t pCharacterId)
    {
        releaseCharacter(pCharacterId);
    });

    mDispatcher.setUnknownPacketHandler(
    [](std::shared_ptr<NetworkSession<connection::Message>> pSession, int pCaseId)
    {
        std::osyncstream(std::cerr) << "[Security] Bilinmeyen Paket ID: " << pCaseId << " IP: " << getRemoteAddress(pSession) << std::endl;
    });

    mNetworkServer.setOnClientConnect(
    [this](std::shared_ptr<NetworkSession<connection::Message>> pSession)
    {
        std::osyncstream(std::cout) << "[Network] Yeni Baglanti: " << getRemoteAddress(pSession) << std::endl;

        pSession->setOnErrorFunction(
        [this](std::shared_ptr<NetworkSession<connection::Message>> pS, const boost::system::error_code& pEc)
        {
            uint64_t tAccountId = pS->getAccountId();

            if (tAccountId == 0)
            {
                return; // never authenticated - nothing registered to clean up
            }

            // remove() returns false when this account already belongs to a
            // NEWER session, i.e. this is the superseded connection
            // AuthController deliberately closed on relogin. The account is
            // still online, so nothing below may run: it would tear down the
            // live session's world state and save a stale position over it.
            if (!mSessionRegistry.remove(tAccountId, pS))
            {
                std::osyncstream(std::cout) << "[Network] Eski oturum kapandi (yeni oturum aktif): " << tAccountId << std::endl;
                return;
            }

            // Unbound right away so nothing is routed to the dead socket;
            // whether the character itself stays in the world for a while is
            // up to the resume grace period.
            uint64_t tCharacterId = pS->getCharacterId();

            if (tCharacterId != 0)
            {
                mSessionRegistry.unbindCharacter(tCharacterId, pS);
            }

            if (mSessionResumeManager.hold(tAccountId))
            {
                std::osyncstream(std::cout) << "[Network] Baglanti koptu, oturum bekletiliyor (hesap): " << tAccountId << std::endl;
                return;
            }

            std::osyncstream(std::cout) << "[Network] Kullanici Dustu (hesap): " << tAccountId << std::endl;

            releaseCharacter(tCharacterId);
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

SessionResumeManager& NetworkManager::getSessionResumeManager()
{
    return mSessionResumeManager;
}

void NetworkManager::setOnUserDisconnected(std::function<void(uint64_t)> pCallback)
{
    mOnUserDisconnected = pCallback;
}

void NetworkManager::releaseCharacter(uint64_t pCharacterId)
{
    if (pCharacterId != 0 && mOnUserDisconnected)
    {
        mOnUserDisconnected(pCharacterId);
    }
}

void NetworkManager::endSession(const std::shared_ptr<NetworkSession<connection::Message>>& pSession)
{
    uint64_t tAccountId = pSession->getAccountId();

    if (tAccountId == 0)
    {
        return;
    }

    uint64_t tCharacterId = pSession->getCharacterId();

    mSessionResumeManager.revoke(tAccountId);

    mSessionRegistry.remove(tAccountId, pSession);

    if (tCharacterId != 0)
    {
        mSessionRegistry.unbindCharacter(tCharacterId, pSession);
        releaseCharacter(tCharacterId);
    }

    pSession->setCharacterId(0);
    pSession->setAccountId(0);

    std::osyncstream(std::cout) << "[Server] Cikis yapildi (hesap: " << tAccountId << ")" << std::endl;
}

void NetworkManager::endDetachedCharacter(uint64_t pAccountId, uint64_t pCharacterId)
{
    mSessionResumeManager.revoke(pAccountId);
    releaseCharacter(pCharacterId);

    std::osyncstream(std::cout) << "[Server] Bekleyen oturum sonlandirildi (hesap: " << pAccountId << ")" << std::endl;
}
