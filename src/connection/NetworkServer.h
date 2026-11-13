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
#include <charconv>
#include <cstdlib>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

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
        , mMaxConnectionsPerIp(readMaxConnectionsPerIp())
    {
        try
        {
            const char* tBind = std::getenv("LAKOT_SERVER_BIND");
            boost::asio::ip::address tAddress = tBind && *tBind ? boost::asio::ip::make_address(tBind) : boost::asio::ip::address(boost::asio::ip::address_v4::any());
            boost::asio::ip::tcp::endpoint tEndpoint(tAddress, pPort);

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

            boost::system::error_code tEndpointError;
            auto tEndpoint = pSocket.remote_endpoint(tEndpointError);

            if (tEndpointError)
            {
                boost::system::error_code tIgnored;
                pSocket.close(tIgnored);
                doAccept();
                return;
            }

            std::string tRemoteIp = tEndpoint.address().to_string();
            bool tAccepted = false;

            {
                std::lock_guard<std::mutex> tLock(mConnectionTracker->mutex);

                auto tIt = mConnectionTracker->perIpConnections.find(tRemoteIp);
                size_t tIpCount = (tIt != mConnectionTracker->perIpConnections.end()) ? tIt->second : 0;
                bool tIpAllowed = (mMaxConnectionsPerIp == 0) || (tIpCount < mMaxConnectionsPerIp);

                if (mConnectionTracker->totalConnections < kMaxConnections && tIpAllowed)
                {
                    ++mConnectionTracker->totalConnections;

                    if (tIt != mConnectionTracker->perIpConnections.end())
                    {
                        ++tIt->second;
                    }
                    else
                    {
                        mConnectionTracker->perIpConnections.emplace(tRemoteIp, 1);
                    }

                    tAccepted = true;
                }
            }

            if (!tAccepted)
            {
                boost::system::error_code tIgnored;
                pSocket.close(tIgnored);
            }
            else
            {
                std::shared_ptr<ConnectionTracker> tTracker = mConnectionTracker;
                std::shared_ptr<void> tLifetimeToken(tTracker.get(),
                [tTracker, tRemoteIp](void*)
                {
                    std::lock_guard<std::mutex> tLock(tTracker->mutex);

                    if (tTracker->totalConnections > 0)
                    {
                        --tTracker->totalConnections;
                    }

                    auto tIt = tTracker->perIpConnections.find(tRemoteIp);
                    if (tIt != tTracker->perIpConnections.end())
                    {
                        if (tIt->second <= 1)
                        {
                            tTracker->perIpConnections.erase(tIt);
                        }
                        else
                        {
                            --tIt->second;
                        }
                    }
                });

                try
                {
                    auto tSession = std::make_shared<NetworkSession<MessageType>>(std::move(pSocket));

                    tSession->setMaxIncomingBytes(kMaxClientMessageBytes);
                    tSession->setIncomingRateLimit(kMaxIncomingMessagesPerSecond, kIncomingMessageBurst);
                    tSession->setLifetimeToken(std::move(tLifetimeToken));

                    if (mOnClientConnect)
                    {
                        mOnClientConnect(tSession);
                    }

                    tSession->startAuthTimeout(kAuthTimeout);
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
    static constexpr size_t kDefaultMaxConnectionsPerIp = 8;
    static constexpr uint32_t kMaxClientMessageBytes = 64 * 1024;
    static constexpr double kMaxIncomingMessagesPerSecond = 100.0;
    static constexpr double kIncomingMessageBurst = 200.0;
    static constexpr std::chrono::seconds kAuthTimeout{60};
    static constexpr std::chrono::milliseconds kAcceptRetryDelay{100};

    struct ConnectionTracker
    {
        std::mutex mutex;
        size_t totalConnections{0};
        std::unordered_map<std::string, size_t> perIpConnections;
    };

    static size_t readMaxConnectionsPerIp()
    {
        const char* tValue = std::getenv("LAKOT_MAX_CONNECTIONS_PER_IP");

        if (!tValue || !*tValue)
        {
            return kDefaultMaxConnectionsPerIp;
        }

        std::string_view tText(tValue);
        size_t tParsed = 0;
        const auto [tEnd, tError] = std::from_chars(tText.data(), tText.data() + tText.size(), tParsed);

        if (tError != std::errc{} || tEnd != tText.data() + tText.size())
        {
            return kDefaultMaxConnectionsPerIp;
        }

        return tParsed;
    }

    boost::asio::io_context mContext;
    boost::asio::ip::tcp::acceptor mAcceptor;
    boost::asio::steady_timer mAcceptRetryTimer;
    std::shared_ptr<ConnectionTracker> mConnectionTracker = std::make_shared<ConnectionTracker>();
    size_t mMaxConnectionsPerIp{kDefaultMaxConnectionsPerIp};

    std::vector<std::thread> mWorkerThreads;

    OnConnectCallback mOnClientConnect;
};

}

#endif
