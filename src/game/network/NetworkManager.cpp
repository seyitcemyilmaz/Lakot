#include "NetworkManager.h"

#include <algorithm>
#include <charconv>
#include <string_view>

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

using namespace lakot;

namespace
{
    std::string getServerHost()
    {
        const char* tValue = SDL_getenv("LAKOT_SERVER_HOST");
        return (tValue && *tValue) ? std::string(tValue) : std::string("127.0.0.1");
    }

    uint16_t getServerPort()
    {
        constexpr uint16_t kDefaultPort = 12345;

        const char* tValue = SDL_getenv("LAKOT_SERVER_PORT");

        if (!tValue || !*tValue)
        {
            return kDefaultPort;
        }

        std::string_view tText(tValue);
        uint16_t tPort = 0;
        auto [tEnd, tError] = std::from_chars(tText.data(), tText.data() + tText.size(), tPort);

        if (tError != std::errc() || tEnd != tText.data() + tText.size() || tPort == 0)
        {
            SDL_Log("LAKOT_SERVER_PORT '%s' is not a valid port, using %u.", tValue, static_cast<unsigned>(kDefaultPort));
            return kDefaultPort;
        }

        return tPort;
    }
}

NetworkManager::NetworkManager()
    : mClient()
    , mDispatcher()
    , mAuthController(this)
    , mWorldController(this)
    , mChatController(this)
    , mInventoryController(this)
{
    mAuthController.initialize();
    mWorldController.initialize();
    mChatController.initialize();
    mInventoryController.initialize();

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
            SDL_Log("[NetworkManager] Mesaj islenemedi: Session yok.");
        }
    });

    mClient.setOnConnectionStatusChanged(
    [this](bool pIsConnected)
    {
        this->onConnectionStatusChanged(pIsConnected);
    });

    mAuthController.setResumeResultCallback(
    [this](bool pIsSuccess)
    {
        this->onResumeResult(pIsSuccess);
    });
}

uint64_t NetworkManager::nextRequestId()
{
    return mNextRequestId.fetch_add(1, std::memory_order_relaxed);
}

void NetworkManager::start()
{
    std::string tHost = getServerHost();
    uint16_t tPort = getServerPort();

    SDL_Log("Connecting to %s:%u", tHost.c_str(), static_cast<unsigned>(tPort));

    mClient.connect(tHost, tPort);
}

void NetworkManager::update()
{
    mClient.update();

    if (mIsReconnecting && std::chrono::steady_clock::now() >= mReconnectDeadline)
    {
        finishReconnect(ConnectionStateType::eLost);
    }
}

void NetworkManager::stop()
{
    if (mAuthController.hasSessionToken())
    {
        logout();
    }

    mClient.stop();
}

void NetworkManager::logout()
{
    mAuthController.sendLogout();
}

void NetworkManager::setConnectionStateCallback(ConnectionStateCallback pCallback)
{
    mConnectionStateCallback = std::move(pCallback);
}

bool NetworkManager::isReconnecting() const
{
    return mIsReconnecting;
}

int NetworkManager::getReconnectSecondsLeft() const
{
    if (!mIsReconnecting)
    {
        return 0;
    }

    auto tLeft = std::chrono::ceil<std::chrono::seconds>(mReconnectDeadline - std::chrono::steady_clock::now());
    return std::max(0, static_cast<int>(tLeft.count()));
}

void NetworkManager::stopReconnecting()
{
    mIsReconnecting = false;
    mIsResumePending = false;
    mAuthController.clearSessionToken();
}

void NetworkManager::onConnectionStatusChanged(bool pIsConnected)
{
    if (!pIsConnected)
    {
        SDL_Log("[NetworkManager] Baglanti koptu, tekrar deneniyor...");

        if (mIsReconnecting)
        {
            mIsResumePending = false; // dropped again mid-resume - retry on the next connect
            return;
        }

        if (!mAuthController.hasSessionToken())
        {
            if (mConnectionStateCallback)
            {
                mConnectionStateCallback(ConnectionStateType::eLost);
            }

            return;
        }

        mIsReconnecting = true;
        mIsResumePending = false;
        mReconnectDeadline = std::chrono::steady_clock::now() + kReconnectWindow;

        if (mConnectionStateCallback)
        {
            mConnectionStateCallback(ConnectionStateType::eReconnecting);
        }

        return;
    }

    if (mIsReconnecting && !mIsResumePending)
    {
        mIsResumePending = true;
        mAuthController.sendResumeSession();
    }
}

void NetworkManager::onResumeResult(bool pIsSuccess)
{
    // Already gave up (timed out or the player logged out). A late success
    // still put the character back into the world server-side, so release it
    // - which also drops the token the response just stored.
    if (!mIsReconnecting)
    {
        if (pIsSuccess)
        {
            mAuthController.sendLogout();
        }

        return;
    }

    finishReconnect(pIsSuccess ? ConnectionStateType::eResumed : ConnectionStateType::eLost);
}

void NetworkManager::finishReconnect(ConnectionStateType pState)
{
    mIsReconnecting = false;
    mIsResumePending = false;

    if (pState == ConnectionStateType::eLost)
    {
        mAuthController.clearSessionToken();
    }

    if (mConnectionStateCallback)
    {
        mConnectionStateCallback(pState);
    }
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

WorldController& NetworkManager::getWorldController()
{
    return mWorldController;
}

ChatController& NetworkManager::getChatController()
{
    return mChatController;
}

InventoryController& NetworkManager::getInventoryController()
{
    return mInventoryController;
}
