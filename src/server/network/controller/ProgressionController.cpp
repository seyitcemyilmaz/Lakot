#include "ProgressionController.h"

#include "WorldController.h"

#include "../../game/Character.h"

using namespace lakot;

namespace
{
    services::progression::AllocateStatStatus toProtoStatus(AllocateStatResultType pResult)
    {
        switch (pResult)
        {
            case AllocateStatResultType::eAllocated: return services::progression::ALLOCATE_STAT_OK;
            case AllocateStatResultType::eNoPoints: return services::progression::ALLOCATE_STAT_NO_POINTS;
            case AllocateStatResultType::eAtCap: return services::progression::ALLOCATE_STAT_AT_CAP;
            case AllocateStatResultType::eInvalid: break;
        }

        return services::progression::ALLOCATE_STAT_INVALID;
    }

    services::progression::ChoosePathStatus toProtoStatus(ChoosePathResultType pResult)
    {
        switch (pResult)
        {
            case ChoosePathResultType::eChosen: return services::progression::CHOOSE_PATH_OK;
            case ChoosePathResultType::eLevelTooLow: return services::progression::CHOOSE_PATH_LEVEL_TOO_LOW;
            case ChoosePathResultType::eAlreadyChosen: return services::progression::CHOOSE_PATH_ALREADY_CHOSEN;
            case ChoosePathResultType::eInvalid: break;
        }

        return services::progression::CHOOSE_PATH_INVALID;
    }

    bool toStatType(services::progression::StatType pStat, StatType& pOut)
    {
        switch (pStat)
        {
            case services::progression::STAT_TYPE_STRENGTH: pOut = StatType::eStrength; return true;
            case services::progression::STAT_TYPE_DEXTERITY: pOut = StatType::eDexterity; return true;
            case services::progression::STAT_TYPE_INTELLIGENCE: pOut = StatType::eIntelligence; return true;
            case services::progression::STAT_TYPE_VITALITY: pOut = StatType::eVitality; return true;
            default: return false;
        }
    }
}

ProgressionController::ProgressionController(NetworkManager& pNetworkManager,
                                             RepositoryManager& pRepositoryManager,
                                             WorldController& pWorldController)
    : BaseController(pNetworkManager, pRepositoryManager)
    , mWorldController(pWorldController)
{

}

void ProgressionController::initialize()
{
    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kAllocateStatRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleAllocateStatRequest(pSession, pMessage);
        }
    );

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kChoosePathRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleChoosePathRequest(pSession, pMessage);
        }
    );
}

void ProgressionController::sendAllocateStatResult(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                                                   uint64_t pReplyTo,
                                                   services::progression::AllocateStatStatus pStatus)
{
    connection::Message tResponse;

    auto* tHeader = tResponse.mutable_response()->mutable_header();
    tHeader->set_reply_to(pReplyTo);
    tHeader->mutable_status()->set_code(pStatus == services::progression::ALLOCATE_STAT_OK ? common::STATUS_OK : common::STATUS_INVALID_REQUEST);

    tResponse.mutable_response()->mutable_allocate_stat_response()->set_status(pStatus);

    pSession->send(tResponse);
}

void ProgressionController::sendChoosePathResult(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                                                 uint64_t pReplyTo,
                                                 services::progression::ChoosePathStatus pStatus)
{
    connection::Message tResponse;

    auto* tHeader = tResponse.mutable_response()->mutable_header();
    tHeader->set_reply_to(pReplyTo);
    tHeader->mutable_status()->set_code(pStatus == services::progression::CHOOSE_PATH_OK ? common::STATUS_OK : common::STATUS_INVALID_REQUEST);

    tResponse.mutable_response()->mutable_choose_path_response()->set_status(pStatus);

    pSession->send(tResponse);
}

void ProgressionController::handleAllocateStatRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tReplyTo = pMessage.request().header().id();
    uint64_t tCharacterId = pSession->getCharacterId();

    StatType tStat = StatType::eStrength;
    uint32_t tAmount = pMessage.request().allocate_stat_request().amount();

    if (tCharacterId == 0 || tAmount == 0 || !toStatType(pMessage.request().allocate_stat_request().stat(), tStat))
    {
        sendAllocateStatResult(pSession, tReplyTo, services::progression::ALLOCATE_STAT_INVALID);
        return;
    }

    mWorldController.withCharacter(tCharacterId,
    [pSession, tReplyTo, tStat, tAmount](Character& pCharacter) -> bool
    {
        AllocateStatResultType tResult = pCharacter.allocateStat(tStat, tAmount);

        sendAllocateStatResult(pSession, tReplyTo, toProtoStatus(tResult));

        return tResult == AllocateStatResultType::eAllocated;
    });
}

void ProgressionController::handleChoosePathRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tReplyTo = pMessage.request().header().id();
    uint64_t tCharacterId = pSession->getCharacterId();
    uint32_t tPathId = pMessage.request().choose_path_request().path_id();

    if (tCharacterId == 0)
    {
        sendChoosePathResult(pSession, tReplyTo, services::progression::CHOOSE_PATH_INVALID);
        return;
    }

    mWorldController.withCharacter(tCharacterId,
    [pSession, tReplyTo, tPathId](Character& pCharacter) -> bool
    {
        ChoosePathResultType tResult = pCharacter.choosePath(tPathId);

        sendChoosePathResult(pSession, tReplyTo, toProtoStatus(tResult));

        return tResult == ChoosePathResultType::eChosen;
    });
}
