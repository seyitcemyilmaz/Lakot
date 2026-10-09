#include "CharacterController.h"
#include "MovementRules.h"

#include <syncstream>

#include "WorldController.h"
#include "InventoryController.h"
#include "../RequestLimits.h"
#include "../../game/ItemCatalog.h"

using namespace lakot;

CharacterController::CharacterController(NetworkManager& pNetworkManager,
                                         RepositoryManager& pRepositoryManager,
                                         WorldController& pWorldController,
                                         InventoryController& pInventoryController,
                                         const ItemCatalog& pItemCatalog,
                                         const MapCatalog& pMapCatalog)
    : BaseController(pNetworkManager, pRepositoryManager)
    , mWorldController(pWorldController)
    , mInventoryController(pInventoryController)
    , mItemCatalog(pItemCatalog)
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
}

void CharacterController::sendCharacterList(const std::shared_ptr<NetworkSession<connection::Message>>& pSession, uint64_t pReplyTo)
{
    uint64_t tAccountId = pSession->getAccountId();

    mRepositoryManager.getCharacterRepository().listByAccount(tAccountId,
    [pSession, pReplyTo](std::vector<CharacterRepository::CharacterRecord> pCharacters, uint32_t pKingdom)
    {
        connection::Message tResponse;

        auto* tHeader = tResponse.mutable_response()->mutable_header();
        tHeader->set_reply_to(pReplyTo);
        tHeader->mutable_status()->set_code(common::STATUS_OK);

        auto* tList = tResponse.mutable_response()->mutable_character_list_response();
        tList->set_max_slots(CharacterRepository::kMaxCharactersPerAccount);
        tList->set_kingdom(pKingdom);

        for (const auto& tCharacter : pCharacters)
        {
            auto* tSummary = tList->add_characters();
            tSummary->set_character_id(tCharacter.characterId);
            tSummary->set_name(tCharacter.name);
            tSummary->set_map_id(tCharacter.mapId);
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
    uint64_t tReplyTo = pMessage.request().header().id();

    auto tSendResult = [pSession, tReplyTo](services::auth::CharacterCreateStatus pStatus,
                                            const CharacterRepository::CharacterRecord* pRecord)
    {
        connection::Message tResponse;

        auto* tHeader = tResponse.mutable_response()->mutable_header();
        tHeader->set_reply_to(tReplyTo);
        tHeader->mutable_status()->set_code(common::STATUS_OK);

        auto* tCreate = tResponse.mutable_response()->mutable_character_create_response();
        tCreate->set_status(pStatus);

        if (pRecord)
        {
            auto* tSummary = tCreate->mutable_character();
            tSummary->set_character_id(pRecord->characterId);
            tSummary->set_name(pRecord->name);
            tSummary->set_map_id(pRecord->mapId);
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

    mRepositoryManager.getCharacterRepository().createCharacter(tAccountId, tName, tKingdom, mMapCatalog,
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

    if (pSession->getCharacterId() != 0)
    {
        connection::Message tResponse;
        auto* tHeader = tResponse.mutable_response()->mutable_header();
        tHeader->set_reply_to(tReplyTo);
        tHeader->mutable_status()->set_code(common::STATUS_INVALID_REQUEST);
        tResponse.mutable_response()->mutable_enter_world_response()
            ->set_status(services::auth::ENTER_WORLD_ERROR);

        pSession->send(tResponse);
        return;
    }

    mRepositoryManager.getCharacterRepository().findOwnedCharacter(tAccountId, tCharacterId,
    [this, pSession, tReplyTo](bool pIsFound, const CharacterRepository::CharacterRecord& pRecord)
    {
        if (!pIsFound)
        {
            connection::Message tResponse;
            auto* tHeader = tResponse.mutable_response()->mutable_header();
            tHeader->set_reply_to(tReplyTo);
            tHeader->mutable_status()->set_code(common::STATUS_UNAUTHORIZED);
            tResponse.mutable_response()->mutable_enter_world_response()
                ->set_status(services::auth::ENTER_WORLD_NOT_FOUND);

            pSession->send(tResponse);
            return;
        }

        mRepositoryManager.getItemRepository().loadCharacterItems(pRecord.characterId,
        [this, pSession, tReplyTo, pRecord](std::vector<ItemInstance> pItems)
        {
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

                std::osyncstream(std::cout) << "[Server] " << pRecord.name << " baslangic noktasina tasindi (harita "
                          << pRecord.mapId << " -> " << tMapId << ")" << std::endl;
            }

            CharacterSnapshot tSnapshot;
            tSnapshot.characterId = pRecord.characterId;
            tSnapshot.name = pRecord.name;
            tSnapshot.kingdom = pRecord.kingdom;
            tSnapshot.stats = pRecord.stats;
            tSnapshot.items = std::move(pItems);

            Character tCharacter(tSnapshot.characterId, tSnapshot.name, mItemCatalog);
            tCharacter.setStats(tSnapshot.stats);
            tCharacter.setItems(tSnapshot.items);

            tSnapshot.stats = tCharacter.getStats();

            connection::Message tResponse;

            auto* tHeader = tResponse.mutable_response()->mutable_header();
            tHeader->set_reply_to(tReplyTo);
            tHeader->mutable_status()->set_code(common::STATUS_OK);

            auto* tEnter = tResponse.mutable_response()->mutable_enter_world_response();
            tEnter->set_status(services::auth::ENTER_WORLD_OK);
            tEnter->set_character_id(pRecord.characterId);
            tEnter->set_name(pRecord.name);
            tEnter->set_map_id(tMapId);
            tEnter->set_pos_x(tX);
            tEnter->set_pos_y(tY);
            tEnter->set_pos_z(tZ);
            tEnter->set_yaw(tYaw);

            fillStats(tCharacter, tEnter->mutable_stats());

            for (const ItemInstance& tItem : tSnapshot.items)
            {
                auto* tItemOut = tEnter->add_items();
                tItemOut->set_template_id(tItem.templateId);
                tItemOut->set_count(tItem.count);
                tItemOut->set_location(static_cast<services::game::ItemLocation>(tItem.location));
                tItemOut->set_slot(tItem.slot);
            }

            pSession->setCharacterId(pRecord.characterId);
            mNetworkManager.getSessionRegistry().bindCharacter(pRecord.characterId, pSession);
            mNetworkManager.getSessionResumeManager().setCharacter(pSession->getAccountId(), pRecord.characterId);

            mInventoryController.sendItemTemplates(pSession);

            pSession->send(tResponse);

            PlayerState tState;
            tState.x = tX;
            tState.y = tY;
            tState.z = tZ;
            tState.yaw = tYaw;

            mWorldController.enterWorld(tSnapshot, tMapId, tState);

            std::osyncstream(std::cout) << "[Server] Dunyaya giris: " << pRecord.name
                      << " (ID: " << pRecord.characterId
                      << ", seviye " << tCharacter.getStats().level
                      << ", " << tSnapshot.items.size() << " esya)" << std::endl;
        });
    });
}

void CharacterController::fillStats(const Character& pCharacter, services::game::CharacterStats* pOut)
{
    const CharacterStats& tStats = pCharacter.getStats();

    pOut->set_level(tStats.level);
    pOut->set_experience(tStats.experience);
    pOut->set_experience_for_next_level(StatFormula::getExperienceForNextLevel(tStats.level));

    pOut->set_health(tStats.health);
    pOut->set_max_health(pCharacter.getMaxHealth());
    pOut->set_mana(tStats.mana);
    pOut->set_max_mana(pCharacter.getMaxMana());

    pOut->set_strength(tStats.strength);
    pOut->set_dexterity(tStats.dexterity);
    pOut->set_intelligence(tStats.intelligence);
    pOut->set_vitality(tStats.vitality);

    pOut->set_attack(pCharacter.getAttack());
    pOut->set_defense(pCharacter.getDefense());

    pOut->set_gold(tStats.gold);
}
