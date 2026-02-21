#ifndef LAKOT_NETWORKSERVER_H
#define LAKOT_NETWORKSERVER_H

#include <vector>
#include <thread>
#include <memory>
#include <functional>

#include <boost/asio.hpp>

#include "NetworkSession.h"

#include "connection_global.h"

namespace lakot
{

template <typename MessageType>
class LAKOT_CONNECTION_EXPORT NetworkServer
{
public:
    using SessionPtr = std::shared_ptr<NetworkSession<MessageType>>;
    using OnConnectCallback = std::function<void(SessionPtr)>;

    ~NetworkServer()
    {
        stop();
    }

    NetworkServer(uint16_t pPort)
        : mAcceptor(mContext)
    {
        try
        {
            boost::asio::ip::tcp::endpoint tEndpoint(boost::asio::ip::tcp::v4(), pPort);

            mAcceptor.open(tEndpoint.protocol());
            mAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            mAcceptor.bind(tEndpoint);
            mAcceptor.listen();
        }
        catch (const std::exception& tException)
        {
            std::cerr << "[NetworkServer] Baslatma Hatasi: " << tException.what() << std::endl;
            throw;
        }
    }

    // Non-Blocking
    void start(size_t pThreadCount = 1)
    {
        doAccept();

        for (size_t i = 0; i < pThreadCount; i++)
        {
            mWorkerThreads.emplace_back(
            [this]()
            {
                mContext.run();
            });
        }
    }

    // Blocking
    void run(size_t pThreadCount = 1)
    {
        doAccept();

        for (size_t i = 0; i < pThreadCount; i++)
        {
            mWorkerThreads.emplace_back(
            [this]()
            {
                mContext.run();
            });
        }

        for (auto& tThread : mWorkerThreads)
        {
            if (tThread.joinable())
            {
                tThread.join();
            }
        }
    }

    void stop()
    {
        if (!mContext.stopped())
        {
            mContext.stop();
        }

        for (auto& tThread : mWorkerThreads)
        {
            if (tThread.joinable())
            {
                tThread.join();
            }
        }

        mWorkerThreads.clear();
    }

    void setOnClientConnect(OnConnectCallback pCallback)
    {
        mOnClientConnect = pCallback;
    }

private:
    void doAccept()
    {
        mAcceptor.async_accept(
        [this](boost::system::error_code pErrorCode, boost::asio::ip::tcp::socket pSocket)
        {
            if (pErrorCode == boost::asio::error::operation_aborted)
            {
                return;
            }

            if (!pErrorCode)
            {
                auto tSession = std::make_shared<NetworkSession<MessageType>>(std::move(pSocket));

                if (mOnClientConnect)
                {
                    mOnClientConnect(tSession);
                }

                tSession->start();
            }

            doAccept();
        });
    }

    boost::asio::io_context mContext;
    boost::asio::ip::tcp::acceptor mAcceptor;

    std::vector<std::thread> mWorkerThreads;

    OnConnectCallback mOnClientConnect;
};

}

#endif
