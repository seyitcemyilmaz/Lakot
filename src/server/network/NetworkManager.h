#ifndef LAKOT_SERVER_NETWORKMANAGER_H
#define LAKOT_SERVER_NETWORKMANAGER_H

#include <functional>

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
    virtual ~NetworkManager();
    NetworkManager();

    void start();
    void stop();
    void run();

    NetworkServer<connection::Message>& getNetworkServer();
    MessageDispatcher<connection::Message>& getDispatcher();
    SessionRegistry<NetworkSession<connection::Message>>& getSessionRegistry();
    SessionResumeManager& getSessionResumeManager();

    // Single-subscriber hook: removes a character from the world. Invoked
    // when a dropped session's resume grace period runs out, on logout, and
    // when a relogin replaces a session that still had a character.
    void setOnUserDisconnected(std::function<void(uint64_t)> pCallback);

    void releaseCharacter(uint64_t pCharacterId);

private:
    NetworkServer<connection::Message> mNetworkServer;
    MessageDispatcher<connection::Message> mDispatcher;
    SessionRegistry<NetworkSession<connection::Message>> mSessionRegistry;
    SessionResumeManager mSessionResumeManager;

    std::function<void(uint64_t)> mOnUserDisconnected;
};

}

#endif
