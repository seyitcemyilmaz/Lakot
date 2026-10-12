#ifndef LAKOT_NETWORKSERVER_H
#define LAKOT_NETWORKSERVER_H

#include <atomic>
#include <chrono>
#include <exception>
#include <iostream>
#include <vector>
#include <thread>
#include <memory>
#include <functional>
#include <syncstream>

#include <boost/asio.hpp>

#include "NetworkSession.h"

#include "connection_global.h"

namespace lakot
{

template <typename MessageType>
class LAKOT_CONNECTION_EXPORT NetworkServer
{
public:
    // For timers that belong with the network layer (e.g. session resume
    // grace periods). Handlers run on the worker pool.
    boost::asio::io_context& getIOContext()
    {
        return mContext;
    }

    using SessionPtr = std::shared_ptr<NetworkSession<MessageType>>;
    using OnConnectCallback = std::function<void(SessionPtr)>;

    ~NetworkServer()
    {
        stop();
    }

    NetworkServer(uint16_t pPort)
        : mAcceptor(mContext)
        , mAcceptRetryTimer(mContext)
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
            std::osyncstream(std::cerr) << "[NetworkServer] Baslatma Hatasi: " << tException.what() << std::endl;
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
                runWorker();
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
                runWorker();
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
            if (tThread.joinable() && tThread.get_id() != std::this_thread::get_id())
            {
                tThread.join();
            }
            else if (tThread.joinable())
            {
                tThread.detach();
            }
        }

        mWorkerThreads.clear();
    }

    void setOnClientConnect(OnConnectCallback pCallback)
    {
        mOnClientConnect = pCallback;
    }

private:
    void runWorker()
    {
        while (true)
        {
            try
            {
                mContext.run();
                return;
            }
            catch (const std::exception& tException)
            {
                std::osyncstream(std::cerr) << "[NetworkServer] Isleyici istisnasi: " << tException.what() << std::endl;
            }
            catch (...)
            {
                std::osyncstream(std::cerr) << "[NetworkServer] Isleyici bilinmeyen istisna" << std::endl;
            }
        }
    }

    void doAccept()
    {
        mAcceptor.async_accept(
        [this](boost::system::error_code pErrorCode, boost::asio::ip::tcp::socket pSocket)
        {
            if (pErrorCode == boost::asio::error::operation_aborted)
            {
                return;
            }

            if (pErrorCode)
            {
                std::osyncstream(std::cerr) << "[NetworkServer] Kabul hatasi: " << pErrorCode.message() << std::endl;

                mAcceptRetryTimer.expires_after(kAcceptRetryDelay);
                mAcceptRetryTimer.async_wait(
                [this](boost::system::error_code pTimerError)
                {
                    if (!pTimerError)
                    {
                        doAccept();
                    }
                });

                return;
            }

            if (mConnectionCount->load() >= kMaxConnections)
            {
                boost::system::error_code tIgnored;
                pSocket.close(tIgnored);
            }
            else
            {
                try
                {
                    auto tSession = std::make_shared<NetworkSession<MessageType>>(std::move(pSocket));

                    tSession->setMaxIncomingBytes(kMaxClientMessageBytes);

                    ++*mConnectionCount;
                    tSession->setLifetimeToken(std::shared_ptr<void>(mConnectionCount.get(),
                    [tCount = mConnectionCount](void*)
                    {
                        --*tCount;
                    }));

                    if (mOnClientConnect)
                    {
                        mOnClientConnect(tSession);
                    }

                    tSession->start();
                }
                catch (const std::exception& tException)
                {
                    std::osyncstream(std::cerr) << "[NetworkServer] Baglanti hatasi: " << tException.what() << std::endl;
                }
                catch (...)
                {
                    std::osyncstream(std::cerr) << "[NetworkServer] Baglanti bilinmeyen hata" << std::endl;
                }
            }

            doAccept();
        });
    }

    static constexpr size_t kMaxConnections = 4096;
    static constexpr uint32_t kMaxClientMessageBytes = 64 * 1024;
    static constexpr std::chrono::milliseconds kAcceptRetryDelay{100};

    boost::asio::io_context mContext;
    boost::asio::ip::tcp::acceptor mAcceptor;
    boost::asio::steady_timer mAcceptRetryTimer;
    std::shared_ptr<std::atomic<size_t>> mConnectionCount = std::make_shared<std::atomic<size_t>>(0);

    std::vector<std::thread> mWorkerThreads;

    OnConnectCallback mOnClientConnect;
};

}

#endif
