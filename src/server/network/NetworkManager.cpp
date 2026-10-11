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

NetworkManager::NetworkManager(uint16_t pPort)
    : mNetworkServer(pPort)
    , mDispatcher()
    , mSessionRegistry()
    , mSessionResumeManager(mNetworkServer.getIOContext(),
        [this](uint64_t pAccountId, uint64_t pGeneration)
        {
            onHoldExpired(pAccountId, pGeneration);
        })
{
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
        [this](std::shared_ptr<NetworkSession<connection::Message>> pS, const boost::system::error_code&)
        {
            onSessionError(pS);
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

void NetworkManager::detachSession(const SessionPtr& pSession)
{
    pSession->setLoginPending(false);
    pSession->setWorldEntryTicket(0);
    pSession->setCharacterId(0);
    pSession->setAccountId(0);
}

void NetworkManager::onSessionError(const SessionPtr& pSession)
{
    std::lock_guard<std::mutex> tLock(mLifecycleMutex);

    uint64_t tAccountId = pSession->getAccountId();

    if (tAccountId == 0)
    {
        return;
    }

    uint64_t tCharacterId = pSession->getCharacterId();

    if (!mSessionRegistry.remove(tAccountId, pSession))
    {
        std::osyncstream(std::cout) << "[Network] Eski oturum kapandi (yeni oturum aktif): " << tAccountId << std::endl;
        detachSession(pSession);
        return;
    }

    if (tCharacterId != 0)
    {
        mSessionRegistry.unbindCharacter(tCharacterId, pSession);
    }

    detachSession(pSession);

    if (mSessionResumeManager.hold(tAccountId))
    {
        std::osyncstream(std::cout) << "[Network] Baglanti koptu, oturum bekletiliyor (hesap): " << tAccountId << std::endl;
        return;
    }

    std::osyncstream(std::cout) << "[Network] Kullanici Dustu (hesap): " << tAccountId << std::endl;

    releaseCharacter(tCharacterId);
}

void NetworkManager::onHoldExpired(uint64_t pAccountId, uint64_t pGeneration)
{
    std::lock_guard<std::mutex> tLock(mLifecycleMutex);

    releaseCharacter(mSessionResumeManager.expireHold(pAccountId, pGeneration));
}

bool NetworkManager::claimLogin(const SessionPtr& pSession)
{
    std::lock_guard<std::mutex> tLock(mLifecycleMutex);

    if (!pSession->isConnected()
        || pSession->getAccountId() != 0
        || pSession->getCharacterId() != 0
        || pSession->isLoginPending())
    {
        return false;
    }

    pSession->setLoginPending(true);

    return true;
}

void NetworkManager::abandonLogin(const SessionPtr& pSession)
{
    std::lock_guard<std::mutex> tLock(mLifecycleMutex);

    pSession->setLoginPending(false);
}

std::optional<std::string> NetworkManager::beginSession(const SessionPtr& pSession, uint64_t pAccountId)
{
    std::lock_guard<std::mutex> tLock(mLifecycleMutex);

    pSession->setLoginPending(false);

    if (!pSession->isConnected() || pSession->getAccountId() != 0)
    {
        return std::nullopt;
    }

    pSession->setAccountId(pAccountId);

    auto tOldSession = mSessionRegistry.addOrReplace(pAccountId, pSession);

    if (tOldSession && tOldSession != pSession)
    {
        std::osyncstream(std::cout) << "[Server] Cakisan oturum. Eski baglanti kapatiliyor." << std::endl;

        if (uint64_t tOldCharacterId = tOldSession->getCharacterId(); tOldCharacterId != 0)
        {
            mSessionRegistry.unbindCharacter(tOldCharacterId, tOldSession);
        }

        detachSession(tOldSession);
        tOldSession->close();
    }

    SessionResumeManager::IssueResult tIssued = mSessionResumeManager.issueToken(pAccountId);

    releaseCharacter(tIssued.releasedCharacterId);

    return std::move(tIssued.token);
}

std::optional<SessionResumeManager::ResumeResult> NetworkManager::resumeSession(const SessionPtr& pSession, const std::string& pToken)
{
    std::lock_guard<std::mutex> tLock(mLifecycleMutex);

    if (!pSession->isConnected() || pSession->getAccountId() != 0 || pSession->getWorldEntryTicket() != 0 || pSession->isLoginPending())
    {
        return std::nullopt;
    }

    auto tResult = mSessionResumeManager.resume(pToken);

    if (!tResult)
    {
        return std::nullopt;
    }

    pSession->setAccountId(tResult->accountId);

    auto tOldSession = mSessionRegistry.addOrReplace(tResult->accountId, pSession);

    if (tOldSession && tOldSession != pSession)
    {
        if (tResult->characterId != 0)
        {
            mSessionRegistry.unbindCharacter(tResult->characterId, tOldSession);
        }

        detachSession(tOldSession);
        tOldSession->close();
    }

    if (tResult->characterId != 0)
    {
        pSession->setCharacterId(tResult->characterId);
        mSessionRegistry.bindCharacter(tResult->characterId, pSession);
    }

    return tResult;
}

uint64_t NetworkManager::claimWorldEntry(const SessionPtr& pSession)
{
    std::lock_guard<std::mutex> tLock(mLifecycleMutex);

    if (!pSession->isConnected() || pSession->getAccountId() == 0
        || pSession->getCharacterId() != 0 || pSession->getWorldEntryTicket() != 0)
    {
        return 0;
    }

    uint64_t tTicket = ++mNextWorldEntryTicket;

    pSession->setWorldEntryTicket(tTicket);

    return tTicket;
}

bool NetworkManager::attachCharacter(const SessionPtr& pSession, uint64_t pAccountId, uint64_t pTicket, uint64_t pCharacterId,
                                     const std::function<void()>& pOnAttached)
{
    std::lock_guard<std::mutex> tLock(mLifecycleMutex);

    if (pTicket == 0
        || !pSession->isConnected()
        || pSession->getAccountId() != pAccountId
        || pSession->getWorldEntryTicket() != pTicket
        || pSession->getCharacterId() != 0
        || mSessionRegistry.get(pAccountId) != pSession)
    {
        return false;
    }

    pSession->setWorldEntryTicket(0);
    pSession->setCharacterId(pCharacterId);

    mSessionRegistry.bindCharacter(pCharacterId, pSession);
    mSessionResumeManager.setCharacter(pAccountId, pCharacterId);

    if (pOnAttached)
    {
        pOnAttached();
    }

    return true;
}

bool NetworkManager::abandonWorldEntry(const SessionPtr& pSession, uint64_t pTicket)
{
    std::lock_guard<std::mutex> tLock(mLifecycleMutex);

    if (pTicket == 0 || pSession->getWorldEntryTicket() != pTicket)
    {
        return false;
    }

    pSession->setWorldEntryTicket(0);

    return true;
}

void NetworkManager::endSession(const SessionPtr& pSession)
{
    std::lock_guard<std::mutex> tLock(mLifecycleMutex);

    uint64_t tAccountId = pSession->getAccountId();

    if (tAccountId == 0)
    {
        return;
    }

    uint64_t tCharacterId = pSession->getCharacterId();

    uint64_t tHeldCharacterId = mSessionResumeManager.revoke(tAccountId);

    mSessionRegistry.remove(tAccountId, pSession);

    if (tCharacterId != 0)
    {
        mSessionRegistry.unbindCharacter(tCharacterId, pSession);
        releaseCharacter(tCharacterId);
    }

    if (tHeldCharacterId != 0 && tHeldCharacterId != tCharacterId)
    {
        releaseCharacter(tHeldCharacterId);
    }

    detachSession(pSession);

    std::osyncstream(std::cout) << "[Server] Cikis yapildi (hesap: " << tAccountId << ")" << std::endl;
}

void NetworkManager::endDetachedCharacter(uint64_t pAccountId, uint64_t pCharacterId)
{
    std::lock_guard<std::mutex> tLock(mLifecycleMutex);

    if (mSessionRegistry.get(pAccountId))
    {
        return;
    }

    uint64_t tHeldCharacterId = mSessionResumeManager.revoke(pAccountId);
    releaseCharacter(pCharacterId);

    if (tHeldCharacterId != 0 && tHeldCharacterId != pCharacterId)
    {
        releaseCharacter(tHeldCharacterId);
    }

    std::osyncstream(std::cout) << "[Server] Bekleyen oturum sonlandirildi (hesap: " << pAccountId << ")" << std::endl;
}
