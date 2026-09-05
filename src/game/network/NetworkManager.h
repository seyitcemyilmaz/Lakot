#ifndef LAKOT_NETWORKMANAGER_H
#define LAKOT_NETWORKMANAGER_H

#include <connection.pb.h>

#include "NetworkClient.h"
#include "MessageDispatcher.h"

#include "controller/AuthController.h"
#include "controller/WorldController.h"
#include "controller/ChatController.h"

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
    WorldController& getWorldController();
    ChatController& getChatController();

private:
    NetworkClient<connection::Message> mClient;
    MessageDispatcher<connection::Message> mDispatcher;

    AuthController mAuthController;
    WorldController mWorldController;
    ChatController mChatController;
};

}

#endif
