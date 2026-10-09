#ifndef LAKOT_NETWORKCLIENT_H
#define LAKOT_NETWORKCLIENT_H

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>
#include <memory>
#include <mutex>
#include <functional>
#include <string>

#include "NetworkSession.h"
#include "ThreadSafeQueue.hpp"

namespace lakot
{

template <typename MessageType>
class LAKOT_CONNECTION_EXPORT NetworkClient
{
public:
    using OnMessageCallback = std::function<void(const MessageType&)>;
    using OnStatusCallback = std::function<void(bool)>;

    NetworkClient()
        : mWorkGuard(boost::asio::make_work_guard(mIOContext))
        , mResolver(mIOContext)
        , mReconnectTimer(mIOContext)
    {
        mIOThread = std::thread(
        [this]()
        {
            mIOContext.run();
        });
    }

    ~NetworkClient()
    {
        stop();
    }

    void connect(const std::string& pHost, uint16_t pPort)
    {
        mHost = pHost;
        mPort = pPort;
        mShouldReconnect = true;

        boost::asio::post(mIOContext, [this]()
        {
            doConnect();
        });
    }

    void update()
    {
        while (!mIncomingQueue.isEmpty())
        {
            auto tMessage = mIncomingQueue.pop_front();

            if (tMessage && mOnMessageReceived)
            {
                mOnMessageReceived(*tMessage);
            }
        }

        while (!mStatusQueue.isEmpty())
        {
            auto tIsConnected = mStatusQueue.pop_front();

            if (tIsConnected && mOnStatusChanged)
            {
                mOnStatusChanged(*tIsConnected);
            }
        }
    }

    void send(const MessageType& pMessage)
    {
        // Was mSession->iIsConnected() - a method that does not exist. This
        // is a template member, so it only failed to compile if something
        // actually called it, which nothing did; the game reaches the session
        // through getSession() instead.
        auto tSession = getSession();

        if (tSession && tSession->isConnected())
        {
            tSession->send(pMessage);
        }
    }

    void stop()
    {
        mShouldReconnect = false;

        if (auto tSession = getSession())
        {
            auto tDeadline = std::chrono::steady_clock::now() + kFlushTimeout;

            while (tSession->hasPendingWrites() && std::chrono::steady_clock::now() < tDeadline)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }

            tSession->close(); // posts to the session's own strand - safe from here
        }

        // mResolver/mReconnectTimer are NOT cancelled directly here: they are
        // owned by the io_context thread and Asio I/O objects are not
        // thread-safe, so touching them from the caller's thread raced with
        // whatever the io thread was doing with them. Dropping the work guard
        // and stopping the context tears them down anyway, and after the join
        // below no other thread is left to race with.
        mWorkGuard.reset();
        mIOContext.stop();

        if (mIOThread.joinable())
        {
            mIOThread.join();
        }
    }

    // mSession is written by the io_context thread (connect success, error/
    // reconnect) and read by the owner's thread (the game's main loop), so
    // every access goes through this lock - copying a shared_ptr is not
    // atomic, and the unguarded version was a genuine data race, not just a
    // theoretical one.
    std::shared_ptr<NetworkSession<MessageType>> getSession() const
    {
        std::lock_guard<std::mutex> tLock(mSessionMutex);
        return mSession;
    }

    void setOnMessageReceived(OnMessageCallback pCallback)
    {
        mOnMessageReceived = pCallback;
    }

    void setOnConnectionStatusChanged(OnStatusCallback pCallback)
    {
        mOnStatusChanged = pCallback;
    }

private:
    void doConnect()
    {
        mResolver.async_resolve(mHost, std::to_string(mPort),
        [this](boost::system::error_code pErrorCode, boost::asio::ip::tcp::resolver::results_type pResults)
        {
            if (!pErrorCode)
            {
                auto tSocket = std::make_shared<boost::asio::ip::tcp::socket>(mIOContext);

                boost::asio::async_connect(*tSocket, pResults,
                [this, tSocket](boost::system::error_code pErrorCode, boost::asio::ip::tcp::endpoint)
                {
                    if (!pErrorCode)
                    {
                        handleConnectSuccess(std::move(*tSocket));
                    }
                    else
                    {
                        handleConnectFail(pErrorCode);
                    }
                });
            }
            else
            {
                handleConnectFail(pErrorCode);
            }
        });
    }

    void handleConnectSuccess(boost::asio::ip::tcp::socket pSocket)
    {
        auto tSession = std::make_shared<NetworkSession<MessageType>>(std::move(pSocket));

        tSession->setOnMessageFunction(
        [this](std::shared_ptr<NetworkSession<MessageType>>, const MessageType& pMessage)
        {
            mIncomingQueue.push_back(pMessage);
        });

        tSession->setOnErrorFunction(
        [this](std::shared_ptr<NetworkSession<MessageType>> pSession, const boost::system::error_code&)
        {
            // Only the CURRENT session counts - a late error from a
            // superseded one must neither drop its replacement nor report a
            // lost connection.
            {
                std::lock_guard<std::mutex> tLock(mSessionMutex);

                if (mSession != pSession)
                {
                    return;
                }

                mSession.reset();
            }

            mStatusQueue.push_back(false);

            scheduleReconnect();
        });

        {
            std::lock_guard<std::mutex> tLock(mSessionMutex);
            mSession = tSession;
        }

        mReconnectDelay = kMinReconnectDelay;

        tSession->start();

        mStatusQueue.push_back(true);
    }

    void handleConnectFail(const boost::system::error_code& pErrorCode)
    {
        scheduleReconnect();
    }

    void scheduleReconnect()
    {
        if (!mShouldReconnect)
        {
            return;
        }

        // 1, 2, 4, 5, 5... - quick while a drop is likely brief, without
        // hammering a server that is down.
        mReconnectTimer.expires_after(mReconnectDelay);
        mReconnectDelay = std::min(mReconnectDelay * 2, kMaxReconnectDelay);
        mReconnectTimer.async_wait(
        [this](boost::system::error_code pErrorCode)
        {
            if (!pErrorCode && mShouldReconnect)
            {
                doConnect();
            }
        });
    }

    boost::asio::io_context mIOContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> mWorkGuard;
    std::thread mIOThread;

    boost::asio::ip::tcp::resolver mResolver;

    mutable std::mutex mSessionMutex;
    std::shared_ptr<NetworkSession<MessageType>> mSession;

    boost::asio::steady_timer mReconnectTimer;

    static constexpr std::chrono::seconds kMinReconnectDelay{1};
    static constexpr std::chrono::seconds kMaxReconnectDelay{5};
    static constexpr std::chrono::milliseconds kFlushTimeout{500};

    // Only touched on the io_context thread.
    std::chrono::seconds mReconnectDelay{kMinReconnectDelay};

    lakot::ThreadSafeQueue<MessageType> mIncomingQueue;

    // Connection status changes, delivered on the owner's thread by update()
    // like messages are, so the callback may safely touch UI/scene state.
    lakot::ThreadSafeQueue<bool> mStatusQueue;

    OnMessageCallback mOnMessageReceived;
    OnStatusCallback mOnStatusChanged;

    std::string mHost;
    uint16_t mPort{0};

    // Written by stop() on the owner's thread, read by the io thread in
    // scheduleReconnect()/the timer handler.
    std::atomic<bool> mShouldReconnect{false};
};

}

#endif
