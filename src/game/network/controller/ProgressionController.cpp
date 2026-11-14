#include "ProgressionController.h"

#include "../NetworkManager.h"

using namespace lakot;

ProgressionController::ProgressionController(NetworkManager* pNetworkManager)
    : mNetworkManager(pNetworkManager)
{

}

void ProgressionController::initialize()
{
    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kAllocateStatResponse, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleAllocateStatResponse(pSession, pMessage);
        }
    );

    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kChoosePathResponse, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleChoosePathResponse(pSession, pMessage);
        }
    );
}

void ProgressionController::sendAllocateStat(services::progression::StatType pStat, uint32_t pAmount)
{
    auto tSession = mNetworkManager->getClient().getSession();

    if (!tSession)
    {
        return;
    }

    connection::Message tMessage;
    auto* tRequest = tMessage.mutable_request()->mutable_allocate_stat_request();
    tRequest->set_stat(pStat);
    tRequest->set_amount(pAmount);

    mNetworkManager->sendRequest(tMessage);
}

void ProgressionController::sendChoosePath(uint32_t pPathId)
{
    auto tSession = mNetworkManager->getClient().getSession();

    if (!tSession)
    {
        return;
    }

    connection::Message tMessage;
    tMessage.mutable_request()->mutable_choose_path_request()->set_path_id(pPathId);

    mNetworkManager->sendRequest(tMessage);
}

void ProgressionController::setAllocateStatResultCallback(AllocateStatResultCallback pCallback)
{
    mAllocateStatResultCallback = std::move(pCallback);
}

void ProgressionController::setChoosePathResultCallback(ChoosePathResultCallback pCallback)
{
    mChoosePathResultCallback = std::move(pCallback);
}

void ProgressionController::clearCallbacks()
{
    mAllocateStatResultCallback = nullptr;
    mChoosePathResultCallback = nullptr;
}

void ProgressionController::handleAllocateStatResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (mAllocateStatResultCallback)
    {
        mAllocateStatResultCallback(pMessage.response().allocate_stat_response().status());
    }
}

void ProgressionController::handleChoosePathResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (mChoosePathResultCallback)
    {
        mChoosePathResultCallback(pMessage.response().choose_path_response().status());
    }
}
