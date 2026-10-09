#include "Zone.h"

#include "../game/ItemCatalog.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <iostream>
#include <utility>
#include <vector>

using namespace lakot;

Zone::Zone(const MapData& pMap, const MapCatalog& pMapCatalog, const ItemCatalog& pItemCatalog, const MonsterCatalog& pMonsterCatalog)
    : mMap(pMap)
    , mMapCatalog(pMapCatalog)
    , mItemCatalog(pItemCatalog)
    , mMonsterCatalog(pMonsterCatalog)
    , mWorkGuard(boost::asio::make_work_guard(mContext))
    , mTickTimer(mContext)
    , mCombat(pMap, mRegistry, mCharacters, mMonsters)
    , mMonsterBrain(pMap, mRegistry, mCharacters, mMonsters, mCombat)
    , mLoot(pMap, pMonsterCatalog, pItemCatalog, mRegistry, mNextEntityId)
{
    spawnWorldEntities();
}

void Zone::spawnWorldEntities()
{
    for (const MapNpc& tNpc : mMap.getNpcs())
    {
        PlayerState tState;
        tState.x = tNpc.x;
        tState.z = tNpc.z;
        tState.y = mMap.getHeightAt(tNpc.x, tNpc.z) + MovementRules::kBodyHalfHeight;
        tState.yaw = tNpc.yaw;

        uint64_t tId = mNextEntityId++;
        mNpcs[tId] = Npc{ tId, tNpc.model, tNpc.role };
        mRegistry.add(tId, tNpc.name, tState, false);
    }

    std::mt19937 tRandom(mMap.getId());
    std::uniform_real_distribution<float> tUnit(0.0f, 1.0f);

    for (const MapMonsterSpawn& tSpawn : mMap.getMonsterSpawns())
    {
        const MonsterTemplate* tTemplate = mMonsterCatalog.find(tSpawn.monsterId);

        if (!tTemplate)
        {
            std::cout << "[Zone] Bilinmeyen canavar sablonu: " << tSpawn.monsterId << std::endl;
            continue;
        }

        for (uint32_t tIndex = 0; tIndex < tSpawn.count; ++tIndex)
        {
            Monster tMonster;
            tMonster.id = mNextEntityId++;
            tMonster.monsterTemplate = tTemplate;
            tMonster.spawnMinLevel = tSpawn.minLevel;
            tMonster.spawnMaxLevel = tSpawn.maxLevel;
            tMonster.spawnX = tSpawn.x;
            tMonster.spawnZ = tSpawn.z;

            for (int tAttempt = 0; tAttempt < 16; ++tAttempt)
            {
                float tAngle = tUnit(tRandom) * 6.2831853f;
                float tDistance = std::sqrt(tUnit(tRandom)) * tSpawn.radius;
                float tX = tSpawn.x + std::cos(tAngle) * tDistance;
                float tZ = tSpawn.z + std::sin(tAngle) * tDistance;

                if (mMap.isWalkable(tX, tZ) && !mMap.isSafe(tX, tZ))
                {
                    tMonster.spawnX = tX;
                    tMonster.spawnZ = tZ;
                    break;
                }
            }

            auto [tIterator, tInserted] = mMonsters.emplace(tMonster.id, tMonster);
            mMonsterBrain.place(tIterator->second);
        }
    }
}

Zone::~Zone()
{
    stop();
}

void Zone::start(SendFunction pSend, TransferFunction pTransfer, PersistFunction pPersist,
                 const std::string& pScriptDirectory)
{
    if (mThread.joinable())
    {
        return; // already running
    }

    // Assigned before the thread starts, so the tick loop never observes them
    // half-set.
    mSend = std::move(pSend);
    mTransfer = std::move(pTransfer);
    mPersist = std::move(pPersist);

    mThread = std::thread([this, pScriptDirectory]()
    {
        // Constructed here rather than in the constructor: the Lua state must
        // be created and used by one thread only, and this is that thread.
        mScriptEngine = std::make_unique<ScriptEngine>();
        mScriptEngine->loadDirectory(pScriptDirectory);

        scheduleTick();
        mContext.run();
    });
}

void Zone::stop()
{
    if (!mThread.joinable())
    {
        return;
    }

    // Queued last, so it runs after every already-posted enter/leave/state
    // update and therefore sees final positions.
    boost::asio::post(mContext, [this]()
    {
        mTickTimer.cancel();
        persistAll(true);
    });

    // Releasing the guard (rather than context.stop()) lets the queue drain,
    // including the save above - the same reasoning as DatabaseManager::stop().
    mWorkGuard.reset();
    mThread.join();
}

uint32_t Zone::getMapId() const
{
    return mMap.getId();
}

void Zone::enterPlayer(CharacterSnapshot pCharacter, const PlayerState& pState)
{
    boost::asio::post(mContext, [this, pCharacter = std::move(pCharacter), pState]() mutable
    {
        uint64_t tCharacterId = pCharacter.characterId;

        Character tLiveCharacter(tCharacterId, pCharacter.name, mItemCatalog);
        tLiveCharacter.setKingdom(pCharacter.kingdom);
        tLiveCharacter.setStats(pCharacter.stats);
        tLiveCharacter.setItems(std::move(pCharacter.items));

        // erase-then-emplace rather than insert_or_assign: Character holds a
        // reference to the ItemCatalog and is therefore not assignable (by
        // design - it is an owned entity, not a value). The erase still
        // matters: a relogin racing its own cleanup must end up with the
        // freshly loaded character, and a bare emplace would silently keep
        // the stale one.
        mCharacters.erase(tCharacterId);
        auto [tIterator, tInserted] = mCharacters.emplace(tCharacterId, std::move(tLiveCharacter));

        mRegistry.add(tCharacterId, std::move(pCharacter.name), pState);
        mCombat.markVitalsChanged(tCharacterId);

        // Fired after the character is fully in place, so a script can rely on
        // everything about it being readable - and before the first snapshot,
        // so anything the script hands out is already there when the client
        // first sees the world. Anything the script changed is picked up by
        // the dirty flag and persisted like any other change.
        if (mScriptEngine)
        {
            mScriptEngine->onEnterWorld(tIterator->second);
        }

        // Always pushed, script or no script. The EnterWorldResponse was
        // built and sent before the character reached this zone, so anything
        // a script just handed out is not in it - and even with no script at
        // all, this is what makes the client's bag correct from the first
        // frame rather than only after the next change.
        sendInventoryUpdate(tIterator->second);

        // Entering counts as an accepted position, so the very first update
        // after a login or a portal is measured from here rather than being
        // handed the full idle budget.
        mLastAcceptedAt[tCharacterId] = std::chrono::steady_clock::now();
        mPendingCorrections.erase(tCharacterId);
    });
}

void Zone::leavePlayer(uint64_t pCharacterId)
{
    boost::asio::post(mContext, [this, pCharacterId]()
    {
        // Unconditionally, not only when dirty - this is the last chance to
        // write this character before it leaves memory.
        persistCharacter(pCharacterId);

        mCharacters.erase(pCharacterId);
        mRegistry.remove(pCharacterId);
        mCombat.forget(pCharacterId);

        mLastAcceptedAt.erase(pCharacterId);
        mPendingCorrections.erase(pCharacterId);
        mLastPickupAt.erase(pCharacterId);
    });
}

void Zone::resumePlayer(uint64_t pCharacterId)
{
    boost::asio::post(mContext, [this, pCharacterId]()
    {
        auto tIterator = mCharacters.find(pCharacterId);

        if (tIterator == mCharacters.end())
        {
            return;
        }

        mRegistry.resetView(pCharacterId);
        mCombat.markVitalsChanged(pCharacterId);

        // Goes out as self_correction in the next snapshot, which is how the
        // client learns where the server has it.
        mPendingCorrections.insert(pCharacterId);

        // The time spent disconnected is not movement budget.
        mLastAcceptedAt[pCharacterId] = std::chrono::steady_clock::now();

        sendInventoryUpdate(tIterator->second);
    });
}

void Zone::withCharacter(uint64_t pCharacterId, std::function<bool(Character&)> pAction)
{
    if (!pAction)
    {
        return;
    }

    boost::asio::post(mContext, [this, pCharacterId, pAction = std::move(pAction)]()
    {
        auto tIterator = mCharacters.find(pCharacterId);

        if (tIterator == mCharacters.end())
        {
            return;
        }

        auto tVisualsBefore = getEquipmentVisuals(tIterator->second);

        if (pAction(tIterator->second))
        {
            sendInventoryUpdate(tIterator->second);

            auto tVisualsAfter = getEquipmentVisuals(tIterator->second);

            if (!std::equal(tVisualsBefore.begin(), tVisualsBefore.end(), tVisualsAfter.begin(), tVisualsAfter.end(),
                            [](const auto& pLeft, const auto& pRight)
                            {
                                return pLeft.slot() == pRight.slot()
                                    && pLeft.template_id() == pRight.template_id()
                                    && pLeft.upgrade_level() == pRight.upgrade_level();
                            }))
            {
                mAppearanceChanged.insert(pCharacterId);
            }
        }
    });
}

void Zone::sendInventoryUpdate(const Character& pCharacter)
{
    if (!mSend)
    {
        return;
    }

    connection::Message tMessage;
    auto* tUpdate = tMessage.mutable_response()->mutable_inventory_update();

    for (const ItemInstance& tItem : pCharacter.getInventory().getItems())
    {
        auto* tItemOut = tUpdate->add_items();
        tItemOut->set_template_id(tItem.templateId);
        tItemOut->set_count(tItem.count);
        tItemOut->set_location(static_cast<services::game::ItemLocation>(tItem.location));
        tItemOut->set_slot(tItem.slot);
    }

    const CharacterStats& tStats = pCharacter.getStats();
    auto* tStatsOut = tUpdate->mutable_stats();

    tStatsOut->set_level(tStats.level);
    tStatsOut->set_experience(tStats.experience);
    tStatsOut->set_experience_for_next_level(StatFormula::getExperienceForNextLevel(tStats.level));
    tStatsOut->set_health(tStats.health);
    tStatsOut->set_max_health(pCharacter.getMaxHealth());
    tStatsOut->set_mana(tStats.mana);
    tStatsOut->set_max_mana(pCharacter.getMaxMana());
    tStatsOut->set_strength(tStats.strength);
    tStatsOut->set_dexterity(tStats.dexterity);
    tStatsOut->set_intelligence(tStats.intelligence);
    tStatsOut->set_vitality(tStats.vitality);
    tStatsOut->set_attack(pCharacter.getAttack());
    tStatsOut->set_defense(pCharacter.getDefense());
    tStatsOut->set_gold(tStats.gold);

    mSend(pCharacter.getId(), NetworkSession<connection::Message>::makePacket(tMessage));
}

void Zone::submitState(uint64_t pCharacterId, const PlayerState& pState)
{
    boost::asio::post(mContext, [this, pCharacterId, pState]()
    {
        const PlayerState* tAccepted = mRegistry.getState(pCharacterId);

        if (!tAccepted)
        {
            return; // not on this map (left, or mid-transfer)
        }

        if (mCombat.isDead(pCharacterId))
        {
            mPendingCorrections.insert(pCharacterId);
            return;
        }

        auto tNow = std::chrono::steady_clock::now();
        auto tLastIterator = mLastAcceptedAt.find(pCharacterId);

        double tElapsedSeconds = kMaxBudgetSeconds;

        if (tLastIterator != mLastAcceptedAt.end())
        {
            tElapsedSeconds = std::chrono::duration<double>(tNow - tLastIterator->second).count();
            tElapsedSeconds = std::min(tElapsedSeconds, kMaxBudgetSeconds);
        }

        float tAllowed = kMaxSpeed * static_cast<float>(tElapsedSeconds) + kPositionTolerance;

        float tDx = pState.x - tAccepted->x;
        float tDz = pState.z - tAccepted->z;

        bool tIsTooFar = tDx * tDx + tDz * tDz > tAllowed * tAllowed;
        bool tIsBlocked = !mMap.isMoveAllowed(tAccepted->x, tAccepted->z, pState.x, pState.z);

        float tExpectedY = mMap.getHeightAt(pState.x, pState.z) + MovementRules::kBodyHalfHeight;
        bool tIsWrongHeight = !(std::abs(pState.y - tExpectedY) <= kHeightTolerance);

        if (tIsTooFar || tIsBlocked || tIsWrongHeight)
        {
            // Rejected: keep the last accepted position as the truth and tell
            // the client to snap back to it. mLastAcceptedAt is intentionally
            // left alone - see its declaration.
            mPendingCorrections.insert(pCharacterId);
            return;
        }

        mLastAcceptedAt[pCharacterId] = tNow;
        mRegistry.setState(pCharacterId, pState);
    });
}

void Zone::sendNearby(uint64_t pOriginCharacterId, Packet pPacket)
{
    if (!pPacket)
    {
        return;
    }

    boost::asio::post(mContext, [this, pOriginCharacterId, pPacket = std::move(pPacket)]()
    {
        if (!mSend)
        {
            return;
        }

        for (uint64_t tOtherId : mRegistry.getNearbyPlayerIds(pOriginCharacterId))
        {
            mSend(tOtherId, pPacket);
        }
    });
}

void Zone::scheduleTick()
{
    mTickTimer.expires_after(std::chrono::milliseconds(kTickIntervalMs));
    mTickTimer.async_wait([this](boost::system::error_code pErrorCode)
    {
        if (pErrorCode)
        {
            return; // cancelled by stop()
        }

        tick();
        scheduleTick();
    });
}

void Zone::attack(uint64_t pCharacterId, float pYaw, uint32_t pCombo)
{
    boost::asio::post(mContext, [this, pCharacterId, pYaw, pCombo]()
    {
        mCombat.playerAttack(pCharacterId, pYaw, pCombo, mTime);
    });
}

void Zone::pickUp(uint64_t pCharacterId, uint64_t pEntityId, uint64_t pReplyTo)
{
    boost::asio::post(mContext, [this, pCharacterId, pEntityId, pReplyTo]()
    {
        auto [tLastPickup, tIsFirst] = mLastPickupAt.try_emplace(pCharacterId, mTime);

        if (!tIsFirst)
        {
            if (mTime - tLastPickup->second < kPickupCooldownSeconds)
            {
                return;
            }

            tLastPickup->second = mTime;
        }

        auto tCharacter = mCharacters.find(pCharacterId);
        const PlayerState* tState = mRegistry.getState(pCharacterId);

        if (tCharacter == mCharacters.end() || !tState || !mSend)
        {
            return;
        }

        services::world::PickupResult tResult = mLoot.pickUp(tCharacter->second, *tState, pEntityId, mTime);

        connection::Message tMessage;
        auto* tHeader = tMessage.mutable_response()->mutable_header();
        tHeader->set_reply_to(pReplyTo);
        tHeader->mutable_status()->set_code(tResult == services::world::PICKUP_RESULT_OK ? common::STATUS_OK : common::STATUS_INVALID_REQUEST);
        tMessage.mutable_response()->mutable_pickup_item_response()->set_result(tResult);
        mSend(pCharacterId, NetworkSession<connection::Message>::makePacket(tMessage));

        if (tResult == services::world::PICKUP_RESULT_OK)
        {
            sendInventoryUpdate(tCharacter->second);
        }
    });
}

void Zone::respawnPlayer(uint64_t pCharacterId)
{
    const MapSpawn& tSpawn = mMap.getSpawn();

    PlayerState tState;
    tState.x = tSpawn.x;
    tState.z = tSpawn.z;
    tState.y = mMap.getHeightAt(tSpawn.x, tSpawn.z) + MovementRules::kBodyHalfHeight;
    tState.yaw = tSpawn.yaw;

    mRegistry.setState(pCharacterId, tState);
    mPendingCorrections.insert(pCharacterId);
    mLastAcceptedAt[pCharacterId] = std::chrono::steady_clock::now();
}

void Zone::fillSelfVitals(const Character& pCharacter, services::world::SelfVitals* pOut) const
{
    const CharacterStats& tStats = pCharacter.getStats();
    pOut->set_health(tStats.health);
    pOut->set_max_health(pCharacter.getMaxHealth());
    pOut->set_level(tStats.level);
    pOut->set_experience(tStats.experience);
    pOut->set_experience_for_next_level(StatFormula::getExperienceForNextLevel(tStats.level));
    pOut->set_respawn_seconds(static_cast<float>(mCombat.getRespawnRemaining(pCharacter.getId(), mTime)));
    pOut->set_kingdom(pCharacter.getKingdom());
}

void Zone::tick()
{
    mTickCounter++;
    mTime += kTickSeconds;

    applyPortals();

    mMonsterBrain.update(mTime, static_cast<float>(kTickSeconds));

    for (uint64_t tRevivedId : mCombat.reviveDuePlayers(mTime))
    {
        respawnPlayer(tRevivedId);
    }

    for (const CombatReward& tReward : mCombat.takeRewards())
    {
        mLoot.drop(tReward, mTime);

        if (auto tCharacter = mCharacters.find(tReward.characterId); tCharacter != mCharacters.end())
        {
            if (mScriptEngine)
            {
                mScriptEngine->onMonsterKilled(tCharacter->second, tReward.monsterTemplateId);

                if (tReward.levelsGained > 0)
                {
                    mScriptEngine->onLevelUp(tCharacter->second, tReward.newLevel);
                }
            }

            sendInventoryUpdate(tCharacter->second);
        }
    }

    mLoot.update(mTime);

    if (mTickCounter % kSnapshotEveryNTicks == 0)
    {
        sendSnapshots();
    }

    if (mTickCounter % kAutosaveEveryNTicks == 0)
    {
        // false: only characters whose stats or items actually changed. An
        // idle zone writes nothing at all, which is the difference between a
        // periodic save costing nothing and costing one UPDATE per player per
        // minute forever.
        persistAll(false);
    }
}

void Zone::applyPortals()
{
    if (!mTransfer)
    {
        return;
    }

    // Collected first, because transferring mutates the registry.
    struct PendingTransfer
    {
        CharacterSnapshot character;
        MapPortal portal;
    };

    std::vector<PendingTransfer> tPending;

    for (uint64_t tCharacterId : mRegistry.getPlayerIds())
    {
        const PlayerState* tState = mRegistry.getState(tCharacterId);

        if (!tState)
        {
            continue;
        }

        if (auto tPortal = mMapCatalog.findPortalAt(mMap.getId(), tState->x, tState->z))
        {
            auto tCharacterIterator = mCharacters.find(tCharacterId);

            if (tCharacterIterator == mCharacters.end())
            {
                continue; // tracked spatially but with no character behind it
            }

            const Character& tCharacter = tCharacterIterator->second;

            // The whole character travels, not just an id - the destination
            // zone reconstructs it from this and never touches the database.
            CharacterSnapshot tSnapshot;
            tSnapshot.characterId = tCharacterId;
            tSnapshot.name = tCharacter.getName();
            tSnapshot.kingdom = tCharacter.getKingdom();
            tSnapshot.stats = tCharacter.getStats();
            tSnapshot.items = tCharacter.getInventory().getItems();

            tPending.push_back({std::move(tSnapshot), *tPortal});
        }
    }

    for (PendingTransfer& tTransfer : tPending)
    {
        uint64_t tCharacterId = tTransfer.character.characterId;

        PlayerState tSpawnState;
        tSpawnState.x = tTransfer.portal.targetX;
        const MapData* tTargetMap = mMapCatalog.getMap(tTransfer.portal.targetMapId);
        tSpawnState.y = tTargetMap ? tTargetMap->getHeightAt(tTransfer.portal.targetX, tTransfer.portal.targetZ) + MovementRules::kBodyHalfHeight : 0.0f;
        tSpawnState.z = tTransfer.portal.targetZ;
        tSpawnState.yaw = tTransfer.portal.targetYaw;

        // Dropped from this zone BEFORE being handed over, so the next
        // snapshot here already reports them as gone to everyone who could
        // see them, and the destination zone is the only one holding them.
        persistCharacter(tCharacterId);

        mRegistry.remove(tCharacterId);
        mCharacters.erase(tCharacterId);
        mCombat.forget(tCharacterId);
        mLastPickupAt.erase(tCharacterId);

        // A portal is a server-authored move, so any correction still queued
        // for this character is stale - the destination zone's own entry
        // becomes the new truth.
        mLastAcceptedAt.erase(tCharacterId);
        mPendingCorrections.erase(tCharacterId);

        mTransfer(tTransfer.character, tTransfer.portal.targetMapId, tSpawnState);
    }
}

bool Zone::fillEntityState(uint64_t pCharacterId, services::world::EntityState* pOut) const
{
    const PlayerState* tState = mRegistry.getState(pCharacterId);

    if (!tState)
    {
        return false;
    }

    pOut->set_entity_id(pCharacterId);
    pOut->mutable_position()->set_x(tState->x);
    pOut->mutable_position()->set_y(tState->y);
    pOut->mutable_position()->set_z(tState->z);
    pOut->set_yaw(tState->yaw);

    return true;
}

bool Zone::fillEntitySpawn(uint64_t pCharacterId, services::world::EntitySpawn* pOut) const
{
    const PlayerState* tState = mRegistry.getState(pCharacterId);

    if (!tState)
    {
        return false;
    }

    pOut->set_entity_id(pCharacterId);
    pOut->mutable_position()->set_x(tState->x);
    pOut->mutable_position()->set_y(tState->y);
    pOut->mutable_position()->set_z(tState->z);
    pOut->set_yaw(tState->yaw);

    if (const std::string* tName = mRegistry.getName(pCharacterId))
    {
        pOut->set_name(*tName);
    }

    if (const GroundItem* tGround = mLoot.find(pCharacterId))
    {
        pOut->set_kind(services::world::ENTITY_KIND_GROUND_ITEM);
        pOut->set_template_id(tGround->templateId);
        pOut->set_count(tGround->count);
        pOut->set_gold(tGround->gold);
        return true;
    }

    pOut->set_health_percent(mCombat.getHealthPercent(pCharacterId));
    pOut->set_is_dead(mCombat.isDead(pCharacterId));

    if (auto tCharacter = mCharacters.find(pCharacterId); tCharacter != mCharacters.end())
    {
        pOut->set_kind(services::world::ENTITY_KIND_PLAYER);
        pOut->set_kingdom(tCharacter->second.getKingdom());
        pOut->set_level(tCharacter->second.getStats().level);

        for (auto& tVisual : getEquipmentVisuals(tCharacter->second))
        {
            *pOut->add_equipment() = std::move(tVisual);
        }
    }
    else if (auto tMonster = mMonsters.find(pCharacterId); tMonster != mMonsters.end())
    {
        pOut->set_kind(services::world::ENTITY_KIND_MONSTER);
        pOut->set_template_id(tMonster->second.monsterTemplate->id);
        pOut->set_level(tMonster->second.stats.level);
    }
    else if (auto tNpc = mNpcs.find(pCharacterId); tNpc != mNpcs.end())
    {
        pOut->set_kind(services::world::ENTITY_KIND_NPC);
        pOut->set_model(tNpc->second.model);
    }

    return true;
}

std::vector<services::world::EquipmentVisual> Zone::getEquipmentVisuals(const Character& pCharacter) const
{
    std::vector<services::world::EquipmentVisual> tVisuals;

    for (uint32_t tSlot = 1; tSlot < static_cast<uint32_t>(EquipSlotType::eCount); ++tSlot)
    {
        const ItemInstance* tItem = pCharacter.getInventory().getEquipped(static_cast<EquipSlotType>(tSlot));
        const ItemTemplate* tTemplate = tItem ? mItemCatalog.find(tItem->templateId) : nullptr;

        if (!tTemplate || tTemplate->model.empty())
        {
            continue;
        }

        services::world::EquipmentVisual& tVisual = tVisuals.emplace_back();
        tVisual.set_slot(tSlot);
        tVisual.set_template_id(tItem->templateId);
        tVisual.set_upgrade_level(0);
    }

    return tVisuals;
}

void Zone::sendSnapshots()
{
    if (!mSend)
    {
        return;
    }

    auto tDeltas = mRegistry.computeDeltas();

    // A player whose movement was rejected still needs a message even when
    // nothing in their view changed - the correction IS the payload. Default-
    // constructing an empty delta here folds them into the same loop instead
    // of needing a second pass with its own message-building code.
    for (uint64_t tCorrectedId : mPendingCorrections)
    {
        tDeltas[tCorrectedId];
    }

    std::unordered_map<uint64_t, std::vector<uint64_t>> tAppearanceFor;

    for (uint64_t tChangedId : mAppearanceChanged)
    {
        if (mCharacters.find(tChangedId) == mCharacters.end())
        {
            continue;
        }

        for (uint64_t tViewerId : mRegistry.getNearbyPlayerIds(tChangedId))
        {
            tAppearanceFor[tViewerId].push_back(tChangedId);
            tDeltas[tViewerId];
        }
    }

    std::vector<CombatRecord> tRecords = mCombat.takeRecords();
    std::unordered_set<uint64_t> tChangedVitals = mCombat.takeChangedVitals();
    std::unordered_map<uint64_t, std::vector<size_t>> tRecordsFor;
    std::unordered_map<uint64_t, std::vector<uint64_t>> tVitalsFor;
    std::vector<uint64_t> tAudience;

    auto tForEachViewerNear = [&](uint64_t pEntityId, auto pAction)
    {
        const PlayerState* tState = mRegistry.getState(pEntityId);

        if (!tState)
        {
            return;
        }

        mRegistry.queryRadius(tState->x, tState->z, WorldRegistry::kInterestRadius, tAudience);

        for (uint64_t tViewerId : tAudience)
        {
            if (mCharacters.count(tViewerId) != 0)
            {
                pAction(tViewerId);
                tDeltas[tViewerId];
            }
        }
    };

    for (size_t tIndex = 0; tIndex < tRecords.size(); ++tIndex)
    {
        tForEachViewerNear(tRecords[tIndex].attackerId, [&](uint64_t pViewerId) { tRecordsFor[pViewerId].push_back(tIndex); });
    }

    for (uint64_t tEntityId : tChangedVitals)
    {
        tForEachViewerNear(tEntityId, [&](uint64_t pViewerId) { tVitalsFor[pViewerId].push_back(tEntityId); });
    }

    // One message per player who has something to hear about - and one
    // serialization each, rather than one per (player x visible player) pair
    // the way the old immediate-push path worked.
    for (auto& [tCharacterId, tDelta] : tDeltas)
    {
        connection::Message tMessage;
        auto* tSnapshot = tMessage.mutable_response()->mutable_world_snapshot();
        tSnapshot->set_tick(mTickCounter);

        for (uint64_t tOtherId : tDelta.entered)
        {
            if (!fillEntitySpawn(tOtherId, tSnapshot->add_entered()))
            {
                tSnapshot->mutable_entered()->RemoveLast();
            }
        }

        for (uint64_t tOtherId : tDelta.moved)
        {
            if (!fillEntityState(tOtherId, tSnapshot->add_moved()))
            {
                tSnapshot->mutable_moved()->RemoveLast();
            }
        }

        for (uint64_t tOtherId : tDelta.left)
        {
            tSnapshot->add_left(tOtherId);
        }

        // Where the server believes this player actually is, after refusing a
        // move it considered impossible. The client snaps to it.
        if (mPendingCorrections.count(tCharacterId) != 0)
        {
            if (!fillEntityState(tCharacterId, tSnapshot->mutable_self_correction()))
            {
                tSnapshot->clear_self_correction();
            }
        }

        auto tAppearance = tAppearanceFor.find(tCharacterId);

        if (tAppearance != tAppearanceFor.end())
        {
            for (uint64_t tOtherId : tAppearance->second)
            {
                // Just spawned in this snapshot: the spawn already carries the new look.
                if (std::find(tDelta.entered.begin(), tDelta.entered.end(), tOtherId) != tDelta.entered.end())
                {
                    continue;
                }

                auto* tAppearanceOut = tSnapshot->add_appearance();
                tAppearanceOut->set_entity_id(tOtherId);

                for (auto& tVisual : getEquipmentVisuals(mCharacters.at(tOtherId)))
                {
                    *tAppearanceOut->add_equipment() = std::move(tVisual);
                }
            }
        }

        if (auto tIndices = tRecordsFor.find(tCharacterId); tIndices != tRecordsFor.end())
        {
            for (size_t tIndex : tIndices->second)
            {
                const CombatRecord& tRecord = tRecords[tIndex];
                auto* tEventOut = tSnapshot->add_combat();
                tEventOut->set_attacker_id(tRecord.attackerId);
                tEventOut->set_combo(tRecord.combo);
                tEventOut->set_target_id(tRecord.targetId);
                tEventOut->set_projectile(tRecord.projectile);

                for (const CombatHit& tHit : tRecord.hits)
                {
                    auto* tHitOut = tEventOut->add_hits();
                    tHitOut->set_target_id(tHit.targetId);
                    tHitOut->set_damage(tHit.damage);
                    tHitOut->set_killed(tHit.killed);
                }
            }
        }

        if (auto tIds = tVitalsFor.find(tCharacterId); tIds != tVitalsFor.end())
        {
            for (uint64_t tEntityId : tIds->second)
            {
                auto* tVitalsOut = tSnapshot->add_vitals();
                tVitalsOut->set_entity_id(tEntityId);
                tVitalsOut->set_health_percent(mCombat.getHealthPercent(tEntityId));
                tVitalsOut->set_is_dead(mCombat.isDead(tEntityId));
            }
        }

        if (tChangedVitals.count(tCharacterId) != 0)
        {
            if (auto tSelf = mCharacters.find(tCharacterId); tSelf != mCharacters.end())
            {
                fillSelfVitals(tSelf->second, tSnapshot->mutable_self_vitals());
            }
        }

        mSend(tCharacterId, NetworkSession<connection::Message>::makePacket(tMessage));
    }

    mPendingCorrections.clear();
    mAppearanceChanged.clear();
}

void Zone::persistCharacter(uint64_t pCharacterId)
{
    if (!mPersist)
    {
        return;
    }

    auto tCharacterIterator = mCharacters.find(pCharacterId);
    const PlayerState* tState = mRegistry.getState(pCharacterId);

    if (tCharacterIterator == mCharacters.end() || !tState)
    {
        return;
    }

    Character& tCharacter = tCharacterIterator->second;

    PersistData tData;
    tData.characterId = pCharacterId;
    tData.mapId = mMap.getId();
    tData.position = *tState;
    tData.stats = tCharacter.getStats();
    tData.items = tCharacter.getInventory().getItems();

    mPersist(tData);

    tCharacter.clearDirty();
}

void Zone::persistAll(bool pForceAll)
{
    if (!mPersist)
    {
        return;
    }

    // Position changes alone do not set the dirty flag - movement is saved on
    // its own cadence and losing a few seconds of walking to a crash is
    // acceptable, whereas losing a quest reward is not. So the autosave
    // writes characters whose STATS or ITEMS changed, and the full sweep
    // (shutdown, or leaving) writes everyone regardless.
    for (auto& [tCharacterId, tCharacter] : mCharacters)
    {
        if (pForceAll || tCharacter.isDirty())
        {
            persistCharacter(tCharacterId);
        }
    }
}
