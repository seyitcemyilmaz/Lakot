#include "CharacterController.h"
#include "MovementRules.h"

#include <syncstream>

#include "WorldController.h"
#include "InventoryController.h"
#include "../RequestLimits.h"
#include "../../game/ItemCatalog.h"
#include "../../game/CharacterAccess.h"
#include "../../world/StatsMessage.h"
#include "ClassCatalog.h"

using namespace lakot;

CharacterController::CharacterController(NetworkManager& pNetworkManager,
                                         RepositoryManager& pRepositoryManager,
                                         WorldController& pWorldController,
                                         InventoryController& pInventoryController,
                                         const ItemCatalog& pItemCatalog,
                                         const ClassCatalog& pClassCatalog,
                                         const MapCatalog& pMapCatalog)
    : BaseController(pNetworkManager, pRepositoryManager)
    , mWorldController(pWorldController)
    , mInventoryController(pInventoryController)
    , mItemCatalog(pItemCatalog)
    , mClassCatalog(pClassCatalog)
    , mMapCatalog(pMapCatalog)
{

}

void CharacterController::initialize()
{
    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kCharacterListRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleCharacterListRequest(pSession, pMessage);
        }
    );

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kCharacterCreateRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleCharacterCreateRequest(pSession, pMessage);
        }
    );

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kEnterWorldRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleEnterWorldRequest(pSession, pMessage);
        }
    );

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kDeleteCharacterRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleDeleteCharacterRequest(pSession, pMessage);
        }
    );
}

void CharacterController::sendCharacterList(const std::shared_ptr<NetworkSession<connection::Message>>& pSession, uint64_t pReplyTo)
{
    uint64_t tAccountId = pSession->getAccountId();

    mRepositoryManager.getCharacterRepository().listByAccount(tAccountId,
    [pSession, pReplyTo](std::vector<CharacterRepository::CharacterRecord> pCharacters, uint32_t pKingdom)
    {
        connection::Message tResponse = makeResponse(pReplyTo, common::STATUS_OK);

        auto* tList = tResponse.mutable_response()->mutable_character_list_response();
        tList->set_max_slots(CharacterRepository::kMaxCharactersPerAccount);
        tList->set_kingdom(pKingdom);

        for (const auto& tCharacter : pCharacters)
        {
            auto* tSummary = tList->add_characters();
            tSummary->set_character_id(tCharacter.characterId);
            tSummary->set_name(tCharacter.name);
            tSummary->set_map_id(tCharacter.mapId);
            tSummary->set_class_id(tCharacter.stats.classId);
            tSummary->set_level(tCharacter.stats.level);
        }

        pSession->send(tResponse);
    });
}

void CharacterController::handleCharacterListRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (pSession->getAccountId() == 0)
    {
        return;
    }

    sendCharacterList(pSession, pMessage.request().header().id());
}

void CharacterController::handleCharacterCreateRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tAccountId = pSession->getAccountId();

    if (tAccountId == 0)
    {
        return;
    }

    const std::string& tName = pMessage.request().character_create_request().name();
    uint32_t tKingdom = pMessage.request().character_create_request().kingdom();
    uint32_t tClassId = pMessage.request().character_create_request().class_id();
    uint64_t tReplyTo = pMessage.request().header().id();

    auto tSendResult = [pSession, tReplyTo](services::auth::CharacterCreateStatus pStatus,
                                            const CharacterRepository::CharacterRecord* pRecord)
    {
        connection::Message tResponse = makeResponse(tReplyTo, common::STATUS_OK);

        auto* tCreate = tResponse.mutable_response()->mutable_character_create_response();
        tCreate->set_status(pStatus);

        if (pRecord)
        {
            auto* tSummary = tCreate->mutable_character();
            tSummary->set_character_id(pRecord->characterId);
            tSummary->set_name(pRecord->name);
            tSummary->set_map_id(pRecord->mapId);
            tSummary->set_class_id(pRecord->stats.classId);
            tSummary->set_level(pRecord->stats.level);
        }

        pSession->send(tResponse);
    };

    if (tName.size() < RequestLimits::kMinUsernameLength
        || tName.size() > RequestLimits::kMaxUsernameLength
        || !RequestLimits::hasOnlyPrintableCharacters(tName))
    {
        tSendResult(services::auth::CHARACTER_CREATE_NAME_INVALID, nullptr);
        return;
    }

    const ClassDefinition* tClass = mClassCatalog.find(tClassId);

    if (!tClass)
    {
        tSendResult(services::auth::CHARACTER_CREATE_INVALID_CLASS, nullptr);
        return;
    }

    CharacterStats tStartStats;
    tStartStats.classId = tClass->id;
    tStartStats.strength = static_cast<int32_t>(tClass->startStrength);
    tStartStats.dexterity = static_cast<int32_t>(tClass->startDexterity);
    tStartStats.intelligence = static_cast<int32_t>(tClass->startIntelligence);
    tStartStats.vitality = static_cast<int32_t>(tClass->startVitality);

    mRepositoryManager.getCharacterRepository().createCharacter(tAccountId, tName, tKingdom, tStartStats, mMapCatalog,
    [tSendResult](CharacterRepository::CreateResult pResult, const CharacterRepository::CharacterRecord& pRecord)
    {
        switch (pResult)
        {
            case CharacterRepository::CreateResult::eCreated:
                tSendResult(services::auth::CHARACTER_CREATE_OK, &pRecord);
                break;
            case CharacterRepository::CreateResult::eNameInUse:
                tSendResult(services::auth::CHARACTER_CREATE_NAME_IN_USE, nullptr);
                break;
            case CharacterRepository::CreateResult::eNameInvalid:
                tSendResult(services::auth::CHARACTER_CREATE_NAME_INVALID, nullptr);
                break;
            case CharacterRepository::CreateResult::eNoFreeSlot:
                tSendResult(services::auth::CHARACTER_CREATE_NO_FREE_SLOT, nullptr);
                break;
            case CharacterRepository::CreateResult::eInvalidKingdom:
                tSendResult(services::auth::CHARACTER_CREATE_INVALID_KINGDOM, nullptr);
                break;
            default:
                tSendResult(services::auth::CHARACTER_CREATE_DATABASE_ERROR, nullptr);
                break;
        }
    });
}

void CharacterController::handleEnterWorldRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tAccountId = pSession->getAccountId();

    if (tAccountId == 0)
    {
        return;
    }

    uint64_t tCharacterId = pMessage.request().enter_world_request().character_id();
    uint64_t tReplyTo = pMessage.request().header().id();

    uint64_t tTicket = mNetworkManager.claimWorldEntry(pSession);

    if (tTicket == 0)
    {
        sendEnterWorldFailure(pSession, tReplyTo, common::STATUS_INVALID_REQUEST, services::auth::ENTER_WORLD_ERROR);
        return;
    }

    mWorldController.runAfterRelease(tCharacterId, [this, pSession, tAccountId, tCharacterId, tReplyTo, tTicket](bool pIsSaved)
    {
        if (!pIsSaved)
        {
            if (mNetworkManager.abandonWorldEntry(pSession, tTicket))
            {
                sendEnterWorldFailure(pSession, tReplyTo, common::STATUS_ERROR, services::auth::ENTER_WORLD_ERROR);
            }

            return;
        }

        mRepositoryManager.getCharacterRepository().loadOwnedCharacter(tAccountId, tCharacterId,
        [this, pSession, tReplyTo, tAccountId, tTicket](CharacterRepository::LoadResult pResult, const CharacterRepository::CharacterRecord& pRecord, std::vector<ItemInstance> pItems)
        {
            if (pResult != CharacterRepository::LoadResult::eLoaded)
            {
                if (mNetworkManager.abandonWorldEntry(pSession, tTicket))
                {
                    if (pResult == CharacterRepository::LoadResult::eNotFound)
                    {
                        sendEnterWorldFailure(pSession, tReplyTo, common::STATUS_UNAUTHORIZED, services::auth::ENTER_WORLD_NOT_FOUND);
                    }
                    else
                    {
                        sendEnterWorldFailure(pSession, tReplyTo, common::STATUS_ERROR, services::auth::ENTER_WORLD_ERROR);
                    }
                }

                return;
            }

            if (!mClassCatalog.find(pRecord.stats.classId))
            {
                std::osyncstream(std::cerr) << "[Server] " << pRecord.name << " has an unknown class: " << pRecord.stats.classId << std::endl;

                if (mNetworkManager.abandonWorldEntry(pSession, tTicket))
                {
                    sendEnterWorldFailure(pSession, tReplyTo, common::STATUS_ERROR, services::auth::ENTER_WORLD_ERROR);
                }

                return;
            }

            uint32_t tMapId = pRecord.mapId;
            float tX = pRecord.x;
            float tY = pRecord.y;
            float tZ = pRecord.z;
            float tYaw = pRecord.yaw;

            const MapData* tMap = mMapCatalog.getMap(tMapId);

            if (!tMap || !tMap->isWalkable(tX, tZ))
            {
                const MapData* tStartMap = mMapCatalog.getStartMap(pRecord.kingdom);
                const MapSpawn& tSpawn = tStartMap->getSpawn();

                tMapId = tStartMap->getId();
                tX = tSpawn.x;
                tZ = tSpawn.z;
                tY = tStartMap->getHeightAt(tX, tZ) + MovementRules::kBodyHalfHeight;
                tYaw = tSpawn.yaw;

                std::osyncstream(std::cout) << "[Server] " << pRecord.name << " moved to start point (map "
                          << pRecord.mapId << " -> " << tMapId << ")" << std::endl;
            }

            CharacterSnapshot tSnapshot;
            tSnapshot.characterId = pRecord.characterId;
            tSnapshot.name = pRecord.name;
            tSnapshot.kingdom = pRecord.kingdom;
            tSnapshot.stats = pRecord.stats;
            tSnapshot.items = std::move(pItems);

            Character tCharacter(tSnapshot.characterId, tSnapshot.name, mItemCatalog, mClassCatalog);
            tCharacter.setItems(tSnapshot.items);
            tCharacter.setStats(tSnapshot.stats);

            tSnapshot.stats = tCharacter.getStats();

            connection::Message tResponse = makeResponse(tReplyTo, common::STATUS_OK);

            auto* tEnter = tResponse.mutable_response()->mutable_enter_world_response();
            tEnter->set_status(services::auth::ENTER_WORLD_OK);
            tEnter->set_character_id(pRecord.characterId);
            tEnter->set_name(pRecord.name);
            tEnter->set_map_id(tMapId);
            tEnter->set_pos_x(tX);
            tEnter->set_pos_y(tY);
            tEnter->set_pos_z(tZ);
            tEnter->set_yaw(tYaw);

            StatsMessage::fill(tCharacter, tEnter->mutable_stats());

            for (const ItemInstance& tItem : tSnapshot.items)
            {
                auto* tItemOut = tEnter->add_items();
                tItemOut->set_template_id(tItem.templateId);
                tItemOut->set_count(tItem.count);
                tItemOut->set_location(static_cast<services::game::ItemLocation>(tItem.location));
                tItemOut->set_slot(tItem.slot);
            }

            PlayerState tState;
            tState.x = tX;
            tState.y = tY;
            tState.z = tZ;
            tState.yaw = tYaw;

            CharacterAccess tAccess;
            tAccess.accountId = tAccountId;
            tAccess.gmLevel = pRecord.gmLevel;
            tAccess.mutedUntilEpoch = pRecord.mutedUntilEpoch;

            bool tIsAttached = mNetworkManager.attachCharacter(pSession, tAccountId, tTicket, pRecord.characterId,
            [&]()
            {
                mInventoryController.sendItemTemplates(pSession);

                pSession->send(tResponse);

                mWorldController.enterWorld(tSnapshot, tMapId, tState, tAccess);
            });

            if (!tIsAttached)
            {
                std::osyncstream(std::cout) << "[Server] World entry cancelled (session changed): " << pRecord.name << std::endl;
                return;
            }

            std::osyncstream(std::cout) << "[Server] World entry: " << pRecord.name
                      << " (ID: " << pRecord.characterId
                      << ", level " << tCharacter.getStats().level
                      << ", " << tSnapshot.items.size() << " items)" << std::endl;
        });
    });
}

void CharacterController::handleDeleteCharacterRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tAccountId = pSession->getAccountId();

    if (tAccountId == 0)
    {
        return;
    }

    const auto& tRequest = pMessage.request().delete_character_request();
    uint64_t tCharacterId = tRequest.character_id();
    uint64_t tReplyTo = pMessage.request().header().id();

    if (tRequest.confirm_name().size() > RequestLimits::kMaxUsernameLength)
    {
        sendDeleteResult(pSession, tReplyTo, services::auth::DELETE_CHARACTER_NAME_MISMATCH, tCharacterId);
        return;
    }

    uint64_t tTicket = mNetworkManager.claimWorldEntry(pSession);

    if (tTicket == 0)
    {
        sendDeleteResult(pSession, tReplyTo, services::auth::DELETE_CHARACTER_IN_WORLD, tCharacterId);
        return;
    }

    std::string tConfirmName = tRequest.confirm_name();

    mWorldController.runAfterRelease(tCharacterId, [this, pSession, tAccountId, tCharacterId, tReplyTo, tTicket, tConfirmName](bool pIsSaved)
    {
        if (!pIsSaved)
        {
            if (mNetworkManager.abandonWorldEntry(pSession, tTicket))
            {
                sendDeleteResult(pSession, tReplyTo, services::auth::DELETE_CHARACTER_DATABASE_ERROR, tCharacterId);
            }

            return;
        }

        if (mWorldController.isCharacterInWorld(tAccountId, tCharacterId))
        {
            if (mNetworkManager.abandonWorldEntry(pSession, tTicket))
            {
                sendDeleteResult(pSession, tReplyTo, services::auth::DELETE_CHARACTER_IN_WORLD, tCharacterId);
            }

            return;
        }

        mRepositoryManager.getCharacterRepository().deleteCharacter(tAccountId, tCharacterId, tConfirmName,
        [this, pSession, tAccountId, tCharacterId, tReplyTo, tTicket, tConfirmName](CharacterRepository::DeleteResult pResult)
        {
            bool tIsOwner = mNetworkManager.abandonWorldEntry(pSession, tTicket);

            services::auth::DeleteCharacterStatus tStatus = services::auth::DELETE_CHARACTER_DATABASE_ERROR;

            switch (pResult)
            {
                case CharacterRepository::DeleteResult::eDeleted:
                    tStatus = services::auth::DELETE_CHARACTER_OK;
                    std::osyncstream(std::cout) << "[Server] Character deleted (account: " << tAccountId
                                                << ", ID: " << tCharacterId << ", name: " << tConfirmName << ")" << std::endl;
                    break;
                case CharacterRepository::DeleteResult::eNotFound:
                    tStatus = services::auth::DELETE_CHARACTER_NOT_FOUND;
                    break;
                case CharacterRepository::DeleteResult::eNameMismatch:
                    tStatus = services::auth::DELETE_CHARACTER_NAME_MISMATCH;
                    break;
                default:
                    break;
            }

            if (tIsOwner)
            {
                sendDeleteResult(pSession, tReplyTo, tStatus, tCharacterId);
            }
        });
    });
}

void CharacterController::sendDeleteResult(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                                           uint64_t pReplyTo,
                                           services::auth::DeleteCharacterStatus pStatus,
                                           uint64_t pCharacterId)
{
    connection::Message tResponse = makeResponse(pReplyTo, common::STATUS_OK);

    auto* tDelete = tResponse.mutable_response()->mutable_delete_character_response();
    tDelete->set_status(pStatus);
    tDelete->set_character_id(pCharacterId);

    pSession->send(tResponse);
}

void CharacterController::sendEnterWorldFailure(const std::shared_ptr<NetworkSession<connection::Message>>& pSession,
                                                uint64_t pReplyTo,
                                                common::StatusCode pStatusCode,
                                                services::auth::EnterWorldStatus pEnterStatus)
{
    connection::Message tResponse = makeResponse(pReplyTo, pStatusCode);
    tResponse.mutable_response()->mutable_enter_world_response()->set_status(pEnterStatus);

    pSession->send(tResponse);
}
