#ifndef LAKOT_NETWORKCLIENT_H
#define LAKOT_NETWORKCLIENT_H

#include <thread>
#include <memory>
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
    }

    void send(const MessageType& pMessage)
    {
        if (mSession && mSession->iIsConnected())
        {
            mSession->send(pMessage);
        }
    }

    void sendRequest(MessageType& pMessage, std::function<void(const MessageType&)> pCallback)
    {
        if (mSession && mSession->iIsConnected())
        {
            mSession->sendRequest(pMessage, pCallback, 0);
        }
    }

    void stop()
    {
        mShouldReconnect = false;

        mReconnectTimer.cancel();
        mResolver.cancel();

        if (mSession)
        {
            mSession->close();
        }

        mIOContext.stop();

        if (mIOThread.joinable())
        {
            mIOThread.join();
        }
    }

    std::shared_ptr<NetworkSession<MessageType>> getSession() const
    {
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
        mSession = std::make_shared<NetworkSession<MessageType>>(std::move(pSocket));

        mSession->setOnMessageFunction(
        [this](std::shared_ptr<NetworkSession<MessageType>>, const MessageType& pMessage)
        {
            mIncomingQueue.push_back(pMessage);
        });

        mSession->setOnErrorFunction(
        [this](std::shared_ptr<NetworkSession<MessageType>>, const boost::system::error_code&)
        {
            if (mOnStatusChanged)
            {
                mOnStatusChanged(false);
            }

            mSession.reset();
            scheduleReconnect();
        });

        mSession->start();

        if (mOnStatusChanged)
        {
            mOnStatusChanged(true);
        }
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

        mReconnectTimer.expires_after(std::chrono::seconds(5));
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

    std::shared_ptr<NetworkSession<MessageType>> mSession;
    boost::asio::steady_timer mReconnectTimer;

    lakot::ThreadSafeQueue<MessageType> mIncomingQueue;

    OnMessageCallback mOnMessageReceived;
    OnStatusCallback mOnStatusChanged;

    std::string mHost;
    uint16_t mPort{0};

    bool mShouldReconnect{false};
};

}

#endif
