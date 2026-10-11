#ifndef LAKOT_SERVER_NETWORKMANAGER_H
#define LAKOT_SERVER_NETWORKMANAGER_H

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include <connection.pb.h>

#include "NetworkServer.h"
#include "MessageDispatcher.h"
#include "SessionRegistry.h"
#include "NetworkSession.h"

#include "SessionResumeManager.h"

namespace lakot
{

class NetworkManager
{
public:
    using SessionPtr = std::shared_ptr<NetworkSession<connection::Message>>;

    virtual ~NetworkManager();
    NetworkManager();

    void start();
    void stop();
    void run();

    static std::string getRemoteAddress(const std::shared_ptr<NetworkSession<connection::Message>>& pSession);

    NetworkServer<connection::Message>& getNetworkServer();
    MessageDispatcher<connection::Message>& getDispatcher();
    SessionRegistry<NetworkSession<connection::Message>>& getSessionRegistry();

    // Single-subscriber hook: removes a character from the world. Invoked
    // when a dropped session's resume grace period runs out, on logout, and
    // when a relogin replaces a session that still had a character.
    void setOnUserDisconnected(std::function<void(uint64_t)> pCallback);

    // Lock order: mLifecycleMutex, then SessionRegistry / SessionResumeManager / WorldController meta mutex. Never taken by zone threads.
    std::optional<std::string> beginSession(const SessionPtr& pSession, uint64_t pAccountId);
    std::optional<SessionResumeManager::ResumeResult> resumeSession(const SessionPtr& pSession, const std::string& pToken);

    uint64_t claimWorldEntry(const SessionPtr& pSession);

    bool attachCharacter(const SessionPtr& pSession, uint64_t pAccountId, uint64_t pTicket, uint64_t pCharacterId,
                         const std::function<void()>& pOnAttached);

    bool abandonWorldEntry(const SessionPtr& pSession, uint64_t pTicket);

    void endSession(const SessionPtr& pSession);
    void endDetachedCharacter(uint64_t pAccountId, uint64_t pCharacterId);

private:
    NetworkServer<connection::Message> mNetworkServer;
    MessageDispatcher<connection::Message> mDispatcher;
    SessionRegistry<NetworkSession<connection::Message>> mSessionRegistry;
    SessionResumeManager mSessionResumeManager;

    std::function<void(uint64_t)> mOnUserDisconnected;

    std::mutex mLifecycleMutex;
    uint64_t mNextWorldEntryTicket{0};

    void onSessionError(const SessionPtr& pSession);
    void onHoldExpired(uint64_t pAccountId, uint64_t pGeneration);

    void releaseCharacter(uint64_t pCharacterId);
    static void detachSession(const SessionPtr& pSession);
};

}

#endif
