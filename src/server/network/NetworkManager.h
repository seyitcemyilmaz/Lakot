#ifndef LAKOT_SERVER_NETWORKMANAGER_H
#define LAKOT_SERVER_NETWORKMANAGER_H

#include <connection.pb.h>

#include "NetworkServer.h"
#include "MessageDispatcher.h"
#include "SessionRegistry.h"
#include "NetworkSession.h"

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

private:
    NetworkServer<connection::Message> mNetworkServer;
    MessageDispatcher<connection::Message> mDispatcher;
    SessionRegistry<NetworkSession<connection::Message>> mSessionRegistry;
};

}

#endif
