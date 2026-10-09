#ifndef LAKOT_NETWORKMANAGER_H
#define LAKOT_NETWORKMANAGER_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

#include <connection.pb.h>

#include "NetworkClient.h"
#include "MessageDispatcher.h"

#include "controller/AuthController.h"
#include "controller/WorldController.h"
#include "controller/ChatController.h"
#include "controller/InventoryController.h"

namespace lakot
{

enum class ConnectionStateType
{
    eReconnecting,  // an authenticated connection dropped; trying to resume
    eResumed,       // the server handed the same session back
    eLost           // not resumable (no login, refused, or out of time)
};

class NetworkManager
{
public:
    using ConnectionStateCallback = std::function<void(ConnectionStateType)>;

    static constexpr std::chrono::seconds kReconnectWindow{30};

    NetworkManager();

    void start();

    void update();

    void stop();

    // Intentional logout: the server releases the character right away.
    void logout();

    // Invoked on the main thread (from update()). A drop while logged in
    // reports eReconnecting and then exactly one of eResumed / eLost; a drop
    // while not logged in reports eLost directly.
    void setConnectionStateCallback(ConnectionStateCallback pCallback);

    bool isReconnecting() const;
    int getReconnectSecondsLeft() const;

    // Gives up on a resume in progress without reporting eLost.
    void stopReconnecting();

    NetworkClient<connection::Message>& getClient();

    MessageDispatcher<connection::Message>& getDispatcher();

    AuthController& getAuthController();
    WorldController& getWorldController();
    ChatController& getChatController();
    InventoryController& getInventoryController();

    // Monotonic id for common.RequestHeader.id, shared by every controller so
    // ids stay unique across services on one connection. The server echoes it
    // back as ResponseHeader.reply_to, which is how a response is matched to
    // the exact request that caused it (see ChatController::sendDirectMessage).
    // Never returns 0 - the protocol uses 0 as "no correlation".
    uint64_t nextRequestId();

private:
    NetworkClient<connection::Message> mClient;
    MessageDispatcher<connection::Message> mDispatcher;

    std::atomic<uint64_t> mNextRequestId{1};

    ConnectionStateCallback mConnectionStateCallback;

    bool mIsReconnecting{false};
    bool mIsResumePending{false};
    std::chrono::steady_clock::time_point mReconnectDeadline;

    void onConnectionStatusChanged(bool pIsConnected);
    void onResumeResult(bool pIsSuccess);
    void finishReconnect(ConnectionStateType pState);

    AuthController mAuthController;
    WorldController mWorldController;
    ChatController mChatController;
    InventoryController mInventoryController;
};

}

#endif
