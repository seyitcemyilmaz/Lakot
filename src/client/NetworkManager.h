#ifndef LAKOT_CLIENT_NETWORKMANAGER_H
#define LAKOT_CLIENT_NETWORKMANAGER_H

#include <connection.pb.h>

#include "NetworkClient.h"
#include "MessageDispatcher.h"

class NetworkManager
{
public:
    NetworkManager();

    void start();

    void update();

    void stop();

    lakot::NetworkClient<connection::Message>& getClient();

    lakot::MessageDispatcher<connection::Message>& getDispatcher();

private:
    lakot::NetworkClient<connection::Message> mClient;
    lakot::MessageDispatcher<connection::Message> mDispatcher;
};
#endif
