#ifndef LAKOT_PROGRESSIONCONTROLLER_H
#define LAKOT_PROGRESSIONCONTROLLER_H

#include <cstdint>
#include <functional>
#include <memory>

#include <connection.pb.h>

namespace lakot
{

class NetworkManager;

template <typename MessageType>
class NetworkSession;

class ProgressionController
{
public:
    using AllocateStatResultCallback = std::function<void(services::progression::AllocateStatStatus pStatus)>;
    using ChoosePathResultCallback = std::function<void(services::progression::ChoosePathStatus pStatus)>;

    ProgressionController(NetworkManager* pNetworkManager);

    void initialize();

    void sendAllocateStat(services::progression::StatType pStat, uint32_t pAmount);
    void sendChoosePath(uint32_t pPathId);

    void setAllocateStatResultCallback(AllocateStatResultCallback pCallback);
    void setChoosePathResultCallback(ChoosePathResultCallback pCallback);

    void clearCallbacks();

private:
    NetworkManager* mNetworkManager;

    AllocateStatResultCallback mAllocateStatResultCallback;
    ChoosePathResultCallback mChoosePathResultCallback;

    void handleAllocateStatResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleChoosePathResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
};

}

#endif
