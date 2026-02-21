#ifndef LAKOT_NETWORKMANAGER_H
#define LAKOT_NETWORKMANAGER_H

#include <connection.pb.h>

#include "NetworkClient.h"
#include "MessageDispatcher.h"

#include "controller/AuthController.h"

namespace lakot
{

class NetworkManager
{
public:
    NetworkManager();

    void start();

    void update();

    void stop();

    NetworkClient<connection::Message>& getClient();

    MessageDispatcher<connection::Message>& getDispatcher();

    AuthController& getAuthController();

private:
    NetworkClient<connection::Message> mClient;
    MessageDispatcher<connection::Message> mDispatcher;

    AuthController mAuthController;
};

}

#endif
