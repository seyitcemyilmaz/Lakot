#include "Zone.h"

#include "../game/ItemCatalog.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <iostream>
#include <syncstream>
#include <utility>
#include <vector>

using namespace lakot;

Zone::Zone(const MapData& pMap, const MapCatalog& pMapCatalog, const ItemCatalog& pItemCatalog, const MonsterCatalog& pMonsterCatalog)
    : mMap(pMap)
    , mMapCatalog(pMapCatalog)
    , mItemCatalog(pItemCatalog)
    , mMonsterCatalog(pMonsterCatalog)
    , mStats(pMap.getId(), std::chrono::milliseconds(kTickIntervalMs))
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
            float tSpawnX = tSpawn.x;
            float tSpawnZ = tSpawn.z;

            for (int tAttempt = 0; tAttempt < 16; ++tAttempt)
            {
                float tAngle = tUnit(tRandom) * 6.2831853f;
                float tDistance = std::sqrt(tUnit(tRandom)) * tSpawn.radius;
                float tX = tSpawn.x + std::cos(tAngle) * tDistance;
                float tZ = tSpawn.z + std::sin(tAngle) * tDistance;

                if (mMap.isWalkable(tX, tZ) && !mMap.isSafe(tX, tZ))
                {
                    tSpawnX = tX;
                    tSpawnZ = tZ;
                    break;
                }
            }

            spawnMonster(*tTemplate, tSpawnX, tSpawnZ, tSpawn.minLevel, tSpawn.maxLevel, false);
        }
    }
}

void Zone::spawnMonster(const MonsterTemplate& pTemplate, float pX, float pZ, uint32_t pMinLevel, uint32_t pMaxLevel, bool pIsTemporary)
{
    Monster tMonster;
    tMonster.id = mNextEntityId++;
    tMonster.monsterTemplate = &pTemplate;
    tMonster.spawnMinLevel = pMinLevel;
    tMonster.spawnMaxLevel = pMaxLevel;
    tMonster.spawnX = pX;
    tMonster.spawnZ = pZ;
    tMonster.isTemporary = pIsTemporary;

    auto [tIterator, tInserted] = mMonsters.emplace(tMonster.id, tMonster);
    mMonsterBrain.place(tIterator->second);
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

        mNextTickAt = std::chrono::steady_clock::now();
        scheduleTick();

        while (true)
        {
            try
            {
                mContext.run();
                return;
            }
            catch (const std::exception& tException)
            {
                std::osyncstream(std::cerr) << "[Zone " << mMap.getId() << "] Isleyici istisnasi: " << tException.what() << std::endl;
            }
            catch (...)
            {
                std::osyncstream(std::cerr) << "[Zone " << mMap.getId() << "] Isleyici bilinmeyen istisna" << std::endl;
            }
        }
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
    postTimed([this]()
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
    postTimed([this, pCharacter = std::move(pCharacter), pState]() mutable
    {
        uint64_t tCharacterId = pCharacter.characterId;
        double tPotionCooldownRemaining = pCharacter.potionCooldownRemaining;

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
        mCombat.scheduleRespawnIfDead(tCharacterId, mTime);

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

        mLastPotionAt.erase(tCharacterId);

        if (tPotionCooldownRemaining > 0.0)
        {
            mLastPotionAt[tCharacterId] = mTime + tPotionCooldownRemaining - kPotionCooldownSeconds;
        }
    });
}

void Zone::leavePlayer(uint64_t pCharacterId, ReleasedCallback pOnReleased)
{
    postTimed([this, pCharacterId, pOnReleased = std::move(pOnReleased)]()
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
        mLastPotionAt.erase(pCharacterId);

        if (pOnReleased)
        {
            pOnReleased();
        }
    });
}

void Zone::resumePlayer(uint64_t pCharacterId)
{
    postTimed([this, pCharacterId]()
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

    postTimed([this, pCharacterId, pAction = std::move(pAction)]()
    {
        auto tIterator = mCharacters.find(pCharacterId);

        if (tIterator == mCharacters.end())
        {
            return;
        }

        auto tVisualsBefore = getEquipmentVisuals(tIterator->second);

        if (pAction(tIterator->second))
        {
            mCombat.markVitalsChanged(pCharacterId);
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

bool Zone::isFinite(const PlayerState& pState)
{
    return std::isfinite(pState.x) && std::isfinite(pState.y) && std::isfinite(pState.z) && std::isfinite(pState.yaw);
}

void Zone::submitState(uint64_t pCharacterId, const PlayerState& pRequestedState)
{
    postTimed([this, pCharacterId, tState = pRequestedState]() mutable
    {
        constexpr float kTwoPi = 6.28318530717958647692f;
        const PlayerState* tAccepted = mRegistry.getState(pCharacterId);

        if (!tAccepted)
        {
            return; // not on this map (left, or mid-transfer)
        }

        if (mCombat.isDead(pCharacterId) || !isFinite(tState))
        {
            mPendingCorrections.insert(pCharacterId);
            return;
        }

        tState.yaw = std::remainder(tState.yaw, kTwoPi);

        auto tNow = std::chrono::steady_clock::now();
        auto tLastIterator = mLastAcceptedAt.find(pCharacterId);

        double tElapsedSeconds = kMaxBudgetSeconds;

        if (tLastIterator != mLastAcceptedAt.end())
        {
            tElapsedSeconds = std::chrono::duration<double>(tNow - tLastIterator->second).count();
            tElapsedSeconds = std::min(tElapsedSeconds, kMaxBudgetSeconds);
        }

        float tAllowed = kMaxSpeed * static_cast<float>(tElapsedSeconds) + kPositionTolerance;

        float tDx = tState.x - tAccepted->x;
        float tDz = tState.z - tAccepted->z;

        bool tIsTooFar = tDx * tDx + tDz * tDz > tAllowed * tAllowed;
        bool tIsBlocked = !mMap.isMoveAllowed(tAccepted->x, tAccepted->z, tState.x, tState.z);

        float tExpectedY = mMap.getHeightAt(tState.x, tState.z) + MovementRules::kBodyHalfHeight;
        bool tIsWrongHeight = !(std::abs(tState.y - tExpectedY) <= kHeightTolerance);

        if (tIsTooFar || tIsBlocked || tIsWrongHeight)
        {
            // Rejected: keep the last accepted position as the truth and tell
            // the client to snap back to it. mLastAcceptedAt is intentionally
            // left alone - see its declaration.
            mPendingCorrections.insert(pCharacterId);
            return;
        }

        mLastAcceptedAt[pCharacterId] = tNow;
        mRegistry.setState(pCharacterId, tState);
    });
}

void Zone::sendNearby(uint64_t pOriginCharacterId, Packet pPacket)
{
    if (!pPacket)
    {
        return;
    }

    postTimed([this, pOriginCharacterId, pPacket = std::move(pPacket)]()
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
    constexpr auto tInterval = std::chrono::milliseconds(kTickIntervalMs);

    mNextTickAt += tInterval;

    if (std::chrono::steady_clock::now() - mNextTickAt > tInterval * kMaxLagTicks)
    {
        mNextTickAt = std::chrono::steady_clock::now();
        mStats.recordLagReset();
    }

    mTickTimer.expires_at(mNextTickAt);
    mTickTimer.async_wait([this](boost::system::error_code pErrorCode)
    {
        if (pErrorCode)
        {
            return; // cancelled by stop()
        }

        try
        {
            tick();
        }
        catch (const std::exception& tException)
        {
            std::osyncstream(std::cerr) << "[Zone " << mMap.getId() << "] Tick istisnasi: " << tException.what() << std::endl;
        }

        scheduleTick();
    });
}

void Zone::attack(uint64_t pCharacterId, float pYaw, uint32_t pCombo)
{
    postTimed([this, pCharacterId, pYaw, pCombo]()
    {
        mCombat.playerAttack(pCharacterId, pYaw, pCombo, mTime);
    });
}

void Zone::pickUp(uint64_t pCharacterId, uint64_t pEntityId, uint64_t pReplyTo)
{
    postTimed([this, pCharacterId, pEntityId, pReplyTo]()
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

void Zone::useItem(uint64_t pCharacterId, const ItemTemplate& pTemplate, UseItemCallback pCallback)
{
    if (!pCallback)
    {
        return;
    }

    postTimed([this, pCharacterId, &pTemplate, pCallback = std::move(pCallback)]()
    {
        auto tCharacter = mCharacters.find(pCharacterId);

        if (tCharacter == mCharacters.end())
        {
            pCallback(services::inventory::USE_ITEM_STATUS_INVALID, 0.0f);
            return;
        }

        if (tCharacter->second.isDead())
        {
            pCallback(services::inventory::USE_ITEM_STATUS_DEAD, 0.0f);
            return;
        }

        auto tLastPotion = mLastPotionAt.find(pCharacterId);

        if (tLastPotion != mLastPotionAt.end())
        {
            double tRemaining = kPotionCooldownSeconds - (mTime - tLastPotion->second);

            if (tRemaining > 0.0)
            {
                pCallback(services::inventory::USE_ITEM_STATUS_COOLDOWN, static_cast<float>(tRemaining));
                return;
            }
        }

        switch (tCharacter->second.useConsumable(pTemplate))
        {
        case ConsumeResultType::eConsumed:
            mLastPotionAt[pCharacterId] = mTime;
            mCombat.markVitalsChanged(pCharacterId);
            sendInventoryUpdate(tCharacter->second);
            pCallback(services::inventory::USE_ITEM_STATUS_OK, static_cast<float>(kPotionCooldownSeconds));
            break;

        case ConsumeResultType::eFull:
            pCallback(services::inventory::USE_ITEM_STATUS_FULL, 0.0f);
            break;

        case ConsumeResultType::eNotOwned:
            pCallback(services::inventory::USE_ITEM_STATUS_NOT_OWNED, 0.0f);
            break;

        case ConsumeResultType::eNotUsable:
            pCallback(services::inventory::USE_ITEM_STATUS_INVALID, 0.0f);
            break;
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

    placeCharacter(pCharacterId, tState);
}

void Zone::placeCharacter(uint64_t pCharacterId, const PlayerState& pState)
{
    mRegistry.setState(pCharacterId, pState);
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
    mStats.beginTick();
    mTickCounter++;
    mTime += kTickSeconds;

    mStats.beginSection();
    applyPortals();
    mStats.endSection(ZoneTickStats::SectionType::ePortal);

    mStats.beginSection();
    mMonsterBrain.update(mTime, static_cast<float>(kTickSeconds));
    mStats.endSection(ZoneTickStats::SectionType::eMonster);

    mStats.beginSection();

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
    mStats.endSection(ZoneTickStats::SectionType::eCombatLoot);

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

    mStats.endTick(mCharacters.size());
}

void Zone::applyPortals()
{
    if (!mTransfer)
    {
        return;
    }

    struct PendingTransfer
    {
        uint64_t characterId;
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
            if (mCharacters.find(tCharacterId) == mCharacters.end())
            {
                continue; // tracked spatially but with no character behind it
            }

            tPending.push_back({tCharacterId, *tPortal});
        }
    }

    for (const PendingTransfer& tTransfer : tPending)
    {
        PlayerState tSpawnState;
        tSpawnState.x = tTransfer.portal.targetX;
        const MapData* tTargetMap = mMapCatalog.getMap(tTransfer.portal.targetMapId);
        tSpawnState.y = tTargetMap ? tTargetMap->getHeightAt(tTransfer.portal.targetX, tTransfer.portal.targetZ) + MovementRules::kBodyHalfHeight : 0.0f;
        tSpawnState.z = tTransfer.portal.targetZ;
        tSpawnState.yaw = tTransfer.portal.targetYaw;

        transferOut(tTransfer.characterId, tTransfer.portal.targetMapId, tSpawnState);
    }
}

void Zone::transferOut(uint64_t pCharacterId, uint32_t pTargetMapId, const PlayerState& pSpawnState)
{
    auto tCharacterIterator = mCharacters.find(pCharacterId);

    if (!mTransfer || tCharacterIterator == mCharacters.end())
    {
        return;
    }

    const Character& tCharacter = tCharacterIterator->second;

    CharacterSnapshot tSnapshot;
    tSnapshot.characterId = pCharacterId;
    tSnapshot.name = tCharacter.getName();
    tSnapshot.kingdom = tCharacter.getKingdom();
    tSnapshot.stats = tCharacter.getStats();
    tSnapshot.items = tCharacter.getInventory().getItems();

    auto tLastPotion = mLastPotionAt.find(pCharacterId);

    if (tLastPotion != mLastPotionAt.end())
    {
        tSnapshot.potionCooldownRemaining = std::max(0.0, kPotionCooldownSeconds - (mTime - tLastPotion->second));
    }

    persistCharacter(pCharacterId);

    mRegistry.remove(pCharacterId);
    mCharacters.erase(pCharacterId);
    mCombat.forget(pCharacterId);
    mLastPickupAt.erase(pCharacterId);
    mLastPotionAt.erase(pCharacterId);
    mLastAcceptedAt.erase(pCharacterId);
    mPendingCorrections.erase(pCharacterId);

    mTransfer(mMap.getId(), tSnapshot, pTargetMapId, pSpawnState);
}

void Zone::teleport(uint64_t pCharacterId, uint32_t pMapId, float pX, float pZ, ActionCallback pCallback)
{
    if (!pCallback)
    {
        return;
    }

    postTimed([this, pCharacterId, pMapId, pX, pZ, pCallback = std::move(pCallback)]()
    {
        const PlayerState* tCurrent = mRegistry.getState(pCharacterId);
        const MapData* tTargetMap = pMapId == mMap.getId() ? &mMap : mMapCatalog.getMap(pMapId);

        if (mCharacters.find(pCharacterId) == mCharacters.end() || !tCurrent)
        {
            pCallback(false, "Player is not online.");
            return;
        }

        if (!tTargetMap || !std::isfinite(pX) || !std::isfinite(pZ) || !tTargetMap->isWalkable(pX, pZ))
        {
            pCallback(false, "Destination is not walkable.");
            return;
        }

        const Character& tCharacter = mCharacters.find(pCharacterId)->second;

        if (pMapId != mMap.getId() && tCharacter.isDead())
        {
            pCallback(false, tCharacter.getName() + " is dead. Use /revive.");
            return;
        }

        PlayerState tState;
        tState.x = pX;
        tState.z = pZ;
        tState.y = tTargetMap->getHeightAt(pX, pZ) + MovementRules::kBodyHalfHeight;
        tState.yaw = tCurrent->yaw;

        if (pMapId == mMap.getId())
        {
            placeCharacter(pCharacterId, tState);
        }
        else
        {
            transferOut(pCharacterId, pMapId, tState);
        }

        pCallback(true, "Teleported.");
    });
}

void Zone::queryPosition(uint64_t pCharacterId, PositionCallback pCallback)
{
    if (!pCallback)
    {
        return;
    }

    postTimed([this, pCharacterId, pCallback = std::move(pCallback)]()
    {
        const PlayerState* tState = mCharacters.find(pCharacterId) != mCharacters.end() ? mRegistry.getState(pCharacterId) : nullptr;

        if (!tState)
        {
            pCallback(false, 0, 0.0f, 0.0f);
            return;
        }

        pCallback(true, mMap.getId(), tState->x, tState->z);
    });
}

void Zone::killPlayer(uint64_t pCharacterId, ActionCallback pCallback)
{
    if (!pCallback)
    {
        return;
    }

    postTimed([this, pCharacterId, pCallback = std::move(pCallback)]()
    {
        auto tCharacter = mCharacters.find(pCharacterId);

        if (tCharacter == mCharacters.end())
        {
            pCallback(false, "Player is not online.");
            return;
        }

        const std::string tName = tCharacter->second.getName();

        if (!mCombat.killPlayer(pCharacterId, mTime))
        {
            pCallback(false, tName + " is already dead.");
            return;
        }

        pCallback(true, "Killed " + tName + ".");
    });
}

void Zone::revivePlayer(uint64_t pCharacterId, ActionCallback pCallback)
{
    if (!pCallback)
    {
        return;
    }

    postTimed([this, pCharacterId, pCallback = std::move(pCallback)]()
    {
        auto tCharacter = mCharacters.find(pCharacterId);

        if (tCharacter == mCharacters.end())
        {
            pCallback(false, "Player is not online.");
            return;
        }

        const std::string tName = tCharacter->second.getName();

        if (!mCombat.revivePlayer(pCharacterId))
        {
            pCallback(false, tName + " is not dead.");
            return;
        }

        pCallback(true, "Revived " + tName + ".");
    });
}

void Zone::spawnMonstersNear(uint64_t pCharacterId, const MonsterTemplate& pTemplate, uint32_t pCount, ActionCallback pCallback)
{
    if (!pCallback)
    {
        return;
    }

    postTimed([this, pCharacterId, &pTemplate, pCount, pCallback = std::move(pCallback)]()
    {
        const PlayerState* tState = mCharacters.find(pCharacterId) != mCharacters.end() ? mRegistry.getState(pCharacterId) : nullptr;

        if (!tState)
        {
            pCallback(false, "Player is not online.");
            return;
        }

        if (mMap.isSafe(tState->x, tState->z))
        {
            pCallback(false, "Cannot spawn monsters in a safe zone.");
            return;
        }

        uint32_t tTemporary = static_cast<uint32_t>(std::count_if(mMonsters.begin(), mMonsters.end(),
            [](const auto& pEntry)
            {
                return pEntry.second.isTemporary;
            }));

        if (uint64_t{tTemporary} + pCount > kMaxTemporaryMonsters)
        {
            pCallback(false, "Too many spawned monsters on this map (limit " + std::to_string(kMaxTemporaryMonsters) + ").");
            return;
        }

        const PlayerState tOrigin = *tState;
        std::uniform_real_distribution<float> tUnit(0.0f, 1.0f);
        uint32_t tSpawned = 0;

        for (uint32_t tIndex = 0; tIndex < pCount; ++tIndex)
        {
            for (int tAttempt = 0; tAttempt < 16; ++tAttempt)
            {
                float tAngle = tUnit(mRandom) * 6.2831853f;
                float tDistance = 1.5f + tUnit(mRandom) * (kSpawnNearRadius - 1.5f);
                float tX = tOrigin.x + std::cos(tAngle) * tDistance;
                float tZ = tOrigin.z + std::sin(tAngle) * tDistance;

                if (mMap.isWalkable(tX, tZ) && !mMap.isSafe(tX, tZ))
                {
                    spawnMonster(pTemplate, tX, tZ, 0, 0, true);
                    ++tSpawned;
                    break;
                }
            }
        }

        if (tSpawned == 0)
        {
            pCallback(false, "No room to spawn here.");
            return;
        }

        pCallback(true, "Spawned " + std::to_string(tSpawned) + " x " + pTemplate.name + ".");
    });
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

    mStats.beginSection();
    auto tDeltas = mRegistry.computeDeltas();
    mStats.endSection(ZoneTickStats::SectionType::eDeltas);

    mStats.beginSection();

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
    mSnapshotEncoder.beginRound();

    for (auto& [tCharacterId, tDelta] : tDeltas)
    {
        mSnapshotEncoder.beginMessage(mTickCounter);

        for (uint64_t tOtherId : tDelta.entered)
        {
            mSnapshotEncoder.addEntered(tOtherId, [&](services::world::EntitySpawn& pOut) { return fillEntitySpawn(tOtherId, &pOut); });
        }

        for (uint64_t tOtherId : tDelta.moved)
        {
            mSnapshotEncoder.addMoved(tOtherId, [&](services::world::EntityState& pOut) { return fillEntityState(tOtherId, &pOut); });
        }

        mSnapshotEncoder.addLeft(tDelta.left);

        if (mPendingCorrections.count(tCharacterId) != 0)
        {
            services::world::EntityState tCorrection;

            if (fillEntityState(tCharacterId, &tCorrection))
            {
                mSnapshotEncoder.setSelfCorrection(tCorrection);
            }
        }

        auto tAppearance = tAppearanceFor.find(tCharacterId);

        if (tAppearance != tAppearanceFor.end())
        {
            for (uint64_t tOtherId : tAppearance->second)
            {
                if (std::find(tDelta.entered.begin(), tDelta.entered.end(), tOtherId) != tDelta.entered.end())
                {
                    continue;
                }

                mSnapshotEncoder.addAppearance(tOtherId, [&](services::world::EntityAppearance& pOut)
                {
                    pOut.set_entity_id(tOtherId);

                    for (auto& tVisual : getEquipmentVisuals(mCharacters.at(tOtherId)))
                    {
                        *pOut.add_equipment() = std::move(tVisual);
                    }

                    return true;
                });
            }
        }

        if (auto tIndices = tRecordsFor.find(tCharacterId); tIndices != tRecordsFor.end())
        {
            for (size_t tIndex : tIndices->second)
            {
                mSnapshotEncoder.addCombat(tIndex, [&](services::world::CombatEvent& pOut)
                {
                    const CombatRecord& tRecord = tRecords[tIndex];
                    pOut.set_attacker_id(tRecord.attackerId);
                    pOut.set_combo(tRecord.combo);
                    pOut.set_target_id(tRecord.targetId);
                    pOut.set_projectile(tRecord.projectile);

                    for (const CombatHit& tHit : tRecord.hits)
                    {
                        auto* tHitOut = pOut.add_hits();
                        tHitOut->set_target_id(tHit.targetId);
                        tHitOut->set_damage(tHit.damage);
                        tHitOut->set_killed(tHit.killed);
                    }

                    return true;
                });
            }
        }

        if (auto tIds = tVitalsFor.find(tCharacterId); tIds != tVitalsFor.end())
        {
            for (uint64_t tEntityId : tIds->second)
            {
                mSnapshotEncoder.addVitals(tEntityId, [&](services::world::EntityVitals& pOut)
                {
                    pOut.set_entity_id(tEntityId);
                    pOut.set_health_percent(mCombat.getHealthPercent(tEntityId));
                    pOut.set_is_dead(mCombat.isDead(tEntityId));
                    return true;
                });
            }
        }

        if (tChangedVitals.count(tCharacterId) != 0)
        {
            if (auto tSelf = mCharacters.find(tCharacterId); tSelf != mCharacters.end())
            {
                services::world::SelfVitals tSelfVitals;
                fillSelfVitals(tSelf->second, &tSelfVitals);
                mSnapshotEncoder.setSelfVitals(tSelfVitals);
            }
        }

        Packet tPacket = mSnapshotEncoder.finishMessage();

        if (tPacket)
        {
            mStats.recordPacket(tPacket->size());
        }

        mSend(tCharacterId, std::move(tPacket));
    }

    mPendingCorrections.clear();
    mAppearanceChanged.clear();
    mStats.endSection(ZoneTickStats::SectionType::eBuildSend);
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
