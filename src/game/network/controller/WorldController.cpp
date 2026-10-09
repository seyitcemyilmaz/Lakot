#include "WorldController.h"

#include "../NetworkManager.h"

using namespace lakot;

namespace
{
    lakot::WorldController::EntitySnapshot toSnapshot(const services::world::EntityState& pState)
    {
        const auto& tPosition = pState.position();
        lakot::WorldController::EntitySnapshot tSnapshot{};
        tSnapshot.entityId = pState.entity_id();
        tSnapshot.x = tPosition.x();
        tSnapshot.y = tPosition.y();
        tSnapshot.z = tPosition.z();
        tSnapshot.yaw = pState.yaw();
        return tSnapshot;
    }

    std::vector<lakot::EquippedVisual> toEquipment(const google::protobuf::RepeatedPtrField<services::world::EquipmentVisual>& pEquipment)
    {
        std::vector<lakot::EquippedVisual> tResult;
        tResult.reserve(static_cast<size_t>(pEquipment.size()));

        for (const auto& tVisual : pEquipment)
        {
            tResult.push_back({ static_cast<lakot::EquipSlotType>(tVisual.slot()), { tVisual.template_id(), tVisual.upgrade_level() } });
        }

        return tResult;
    }

    lakot::WorldController::EntitySnapshot toSnapshot(const services::world::EntitySpawn& pSpawn)
    {
        const auto& tPosition = pSpawn.position();
        lakot::WorldController::EntitySnapshot tSnapshot{};
        tSnapshot.entityId = pSpawn.entity_id();
        tSnapshot.x = tPosition.x();
        tSnapshot.y = tPosition.y();
        tSnapshot.z = tPosition.z();
        tSnapshot.yaw = pSpawn.yaw();
        tSnapshot.name = pSpawn.name();
        tSnapshot.equipment = toEquipment(pSpawn.equipment());
        tSnapshot.kind = static_cast<uint32_t>(pSpawn.kind());
        tSnapshot.templateId = pSpawn.template_id();
        tSnapshot.model = pSpawn.model();
        tSnapshot.healthPercent = pSpawn.health_percent();
        tSnapshot.kingdom = pSpawn.kingdom();
        tSnapshot.isDead = pSpawn.is_dead();
        tSnapshot.count = pSpawn.count();
        tSnapshot.gold = pSpawn.gold();
        tSnapshot.level = pSpawn.level();
        return tSnapshot;
    }
}

WorldController::WorldController(NetworkManager* pNetworkManager)
    : mNetworkManager(pNetworkManager)
{

}

void WorldController::initialize()
{
    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kWorldSnapshot, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleWorldSnapshot(pSession, pMessage);
        }
    );

    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kMapChanged, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleMapChanged(pSession, pMessage);
        }
    );

    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kPickupItemResponse, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handlePickupItemResponse(pSession, pMessage);
        }
    );
}

void WorldController::sendPlayerStateUpdate(float pX, float pY, float pZ, float pYaw)
{
    auto tSession = mNetworkManager->getClient().getSession();

    if (!tSession)
    {
        return;
    }

    connection::Message tMessage;
    auto* tUpdate = tMessage.mutable_request()->mutable_player_state_update();
    tUpdate->mutable_position()->set_x(pX);
    tUpdate->mutable_position()->set_y(pY);
    tUpdate->mutable_position()->set_z(pZ);
    tUpdate->set_yaw(pYaw);

    tSession->send(tMessage);
}

void WorldController::sendAttack(float pYaw, uint32_t pCombo)
{
    auto tSession = mNetworkManager->getClient().getSession();

    if (!tSession)
    {
        return;
    }

    connection::Message tMessage;
    auto* tRequest = tMessage.mutable_request()->mutable_attack_request();
    tRequest->set_yaw(pYaw);
    tRequest->set_combo(pCombo);
    tSession->send(tMessage);
}

void WorldController::sendPickup(uint64_t pEntityId)
{
    auto tSession = mNetworkManager->getClient().getSession();

    if (!tSession)
    {
        return;
    }

    connection::Message tMessage;
    tMessage.mutable_request()->mutable_header()->set_id(mNetworkManager->nextRequestId());
    tMessage.mutable_request()->mutable_pickup_item_request()->set_entity_id(pEntityId);
    tSession->send(tMessage);
}

void WorldController::setWorldSnapshotCallback(WorldSnapshotCallback pCallback)
{
    mWorldSnapshotCallback = pCallback;
}

void WorldController::setMapChangedCallback(MapChangedCallback pCallback)
{
    mMapChangedCallback = pCallback;
}

void WorldController::setPickupResultCallback(PickupResultCallback pCallback)
{
    mPickupResultCallback = pCallback;
}

void WorldController::clearCallbacks()
{
    mPickupResultCallback = nullptr;
    mWorldSnapshotCallback = nullptr;
    mMapChangedCallback = nullptr;
}

void WorldController::handleWorldSnapshot(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mWorldSnapshotCallback)
    {
        return;
    }

    const auto& tSnapshot = pMessage.response().world_snapshot();

    WorldSnapshot tResult;
    tResult.tick = tSnapshot.tick();

    tResult.entered.reserve(tSnapshot.entered_size());
    for (const auto& tSpawn : tSnapshot.entered())
    {
        tResult.entered.push_back(toSnapshot(tSpawn));
    }

    tResult.moved.reserve(tSnapshot.moved_size());
    for (const auto& tState : tSnapshot.moved())
    {
        tResult.moved.push_back(toSnapshot(tState));
    }

    tResult.left.reserve(tSnapshot.left_size());
    for (uint64_t tEntityId : tSnapshot.left())
    {
        tResult.left.push_back(tEntityId);
    }

    if (tSnapshot.has_self_correction())
    {
        tResult.selfCorrection = toSnapshot(tSnapshot.self_correction());
    }

    for (const auto& tEvent : tSnapshot.combat())
    {
        CombatSnapshot tCombat{ tEvent.attacker_id(), tEvent.combo(), {} };

        for (const auto& tHit : tEvent.hits())
        {
            tCombat.hits.push_back({ tHit.target_id(), tHit.damage(), tHit.killed() });
        }

        tResult.combat.push_back(std::move(tCombat));
    }

    for (const auto& tVitals : tSnapshot.vitals())
    {
        tResult.vitals.push_back({ tVitals.entity_id(), tVitals.health_percent(), tVitals.is_dead() });
    }

    if (tSnapshot.has_self_vitals())
    {
        const auto& tSelf = tSnapshot.self_vitals();
        tResult.selfVitals = SelfVitalsSnapshot{ tSelf.health(), tSelf.max_health(), tSelf.level(), tSelf.experience(),
                                                 tSelf.experience_for_next_level(), tSelf.respawn_seconds(), tSelf.kingdom() };
    }

    tResult.appearance.reserve(tSnapshot.appearance_size());
    for (const auto& tAppearance : tSnapshot.appearance())
    {
        tResult.appearance.push_back({ tAppearance.entity_id(), toEquipment(tAppearance.equipment()) });
    }

    mWorldSnapshotCallback(tResult);
}

void WorldController::handlePickupItemResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (mPickupResultCallback)
    {
        mPickupResultCallback(pMessage.response().pickup_item_response().result());
    }
}

void WorldController::handleMapChanged(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mMapChangedCallback)
    {
        return;
    }

    const auto& tChanged = pMessage.response().map_changed();
    const auto& tPosition = tChanged.position();

    MapSnapshot tSnapshot{ static_cast<uint32_t>(tChanged.map_id()), tPosition.x(), tPosition.y(), tPosition.z(), tChanged.yaw() };

    mMapChangedCallback(tSnapshot);
}
