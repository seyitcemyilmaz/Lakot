#include "CharacterSaveQueue.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <syncstream>

#include "UnsavedSnapshotWriter.h"

using namespace lakot;

namespace
{
    constexpr std::chrono::milliseconds kRetryBaseDelay{250};
    constexpr std::chrono::milliseconds kRetryMaxDelay{5000};
    constexpr std::chrono::milliseconds kFlushRetryDelay{500};
    constexpr std::chrono::seconds kSummaryInterval{10};
    constexpr uint32_t kMaxPermanentFailures = 3;
    constexpr uint32_t kMaxBackoffShift = 5;
    constexpr double kJitterFraction = 0.2;
}

CharacterSaveQueue::CharacterSaveQueue(CharacterRepository& pRepository)
    : mRepository(pRepository)
    , mRandom(std::random_device{}())
    , mLastSummaryAt(std::chrono::steady_clock::now())
    , mWorkGuard(boost::asio::make_work_guard(mContext))
{
    mThread = std::thread([this]()
    {
        mContext.run();
    });
}

CharacterSaveQueue::~CharacterSaveQueue()
{
    {
        std::lock_guard<std::mutex> tLock(mMutex);
        mIsStopped = true;
    }

    stopContext();
}

void CharacterSaveQueue::stopContext()
{
    mWorkGuard.reset();
    mContext.stop();

    if (mThread.joinable())
    {
        mThread.join();
    }
}

CharacterRepository::CharacterSave CharacterSaveQueue::takeForSend(Entry& pEntry)
{
    pEntry.hasUnsent = false;
    pEntry.isInFlight = true;
    return pEntry.latest;
}

void CharacterSaveQueue::submit(CharacterRepository::CharacterSave pSave)
{
    const uint64_t tCharacterId = pSave.characterId;
    std::optional<CharacterRepository::CharacterSave> tToSend;

    {
        std::lock_guard<std::mutex> tLock(mMutex);

        if (mIsStopped)
        {
            std::osyncstream(std::cerr) << "[SaveQueue] Kapali kuyruga kayit geldi, atlandi: " << tCharacterId << std::endl;
            return;
        }

        Entry& tEntry = mEntries[tCharacterId];
        tEntry.latest = std::move(pSave);
        tEntry.hasUnsent = true;
        ++tEntry.sequence;

        if (!tEntry.isInFlight && !tEntry.isWaitingRetry)
        {
            tToSend = takeForSend(tEntry);
        }
    }

    if (tToSend)
    {
        send(tCharacterId, std::move(*tToSend));
    }
}

void CharacterSaveQueue::whenSettled(uint64_t pCharacterId, SettledCallback pCallback)
{
    {
        std::lock_guard<std::mutex> tLock(mMutex);

        auto tIterator = mEntries.find(pCharacterId);

        if (tIterator != mEntries.end())
        {
            tIterator->second.waiters.push_back(std::move(pCallback));
            return;
        }
    }

    pCallback(true);
}

size_t CharacterSaveQueue::getUnsavedCount() const
{
    std::lock_guard<std::mutex> tLock(mMutex);
    return mEntries.size();
}

void CharacterSaveQueue::send(uint64_t pCharacterId, CharacterRepository::CharacterSave pSave)
{
    mRepository.saveCharacter(std::move(pSave), [this, pCharacterId](CharacterRepository::SaveResult pResult)
    {
        onSaveResult(pCharacterId, pResult);
    });
}

void CharacterSaveQueue::onSaveResult(uint64_t pCharacterId, CharacterRepository::SaveResult pResult)
{
    using SaveResult = CharacterRepository::SaveResult;

    std::optional<CharacterRepository::CharacterSave> tNext;
    std::vector<SettledCallback> tSettled;
    bool tIsSaved = false;

    {
        std::lock_guard<std::mutex> tLock(mMutex);

        auto tIterator = mEntries.find(pCharacterId);

        if (tIterator == mEntries.end())
        {
            return;
        }

        Entry& tEntry = tIterator->second;
        tEntry.isInFlight = false;

        if (pResult == SaveResult::eSaved || pResult == SaveResult::eCharacterGone)
        {
            if (pResult == SaveResult::eCharacterGone)
            {
                std::osyncstream(std::cout) << "[SaveQueue] Karakter artik yok, kayit atlandi: " << pCharacterId << std::endl;
            }
            else if (tEntry.attempt > 0)
            {
                std::osyncstream(std::cout) << "[SaveQueue] Karakter kaydi toparlandi (id: " << pCharacterId
                                            << ", basarisiz deneme: " << tEntry.attempt << ")" << std::endl;
            }

            tEntry.attempt = 0;
            tEntry.permanentFailures = 0;

            if (tEntry.hasUnsent && !mIsStopped)
            {
                tNext = takeForSend(tEntry);
            }
            else
            {
                tSettled = std::move(tEntry.waiters);
                tIsSaved = true;
                mEntries.erase(tIterator);
                mErased.notify_all();
            }
        }
        else
        {
            ++tEntry.attempt;
            logFailure(pCharacterId, tEntry);

            if (pResult == SaveResult::ePermanent)
            {
                ++tEntry.permanentFailures;
            }

            if (tEntry.hasUnsent && !mIsStopped)
            {
                tEntry.permanentFailures = 0;
                tNext = takeForSend(tEntry);
            }
            else if (tEntry.permanentFailures >= kMaxPermanentFailures)
            {
                std::osyncstream(std::cerr) << "[SaveQueue] HATA: Karakter kaydi kalici olarak basarisiz, vazgecildi (id: " << pCharacterId
                                            << ", seviye: " << tEntry.latest.stats.level << ", altin: " << tEntry.latest.stats.gold
                                            << ", esya: " << tEntry.latest.items.size() << ")" << std::endl;

                tSettled = std::move(tEntry.waiters);
                tIsSaved = true;
                mEntries.erase(tIterator);
                mErased.notify_all();
            }
            else
            {
                tEntry.hasUnsent = true;
                tSettled = std::move(tEntry.waiters);

                if (!mIsStopped)
                {
                    armRetry(pCharacterId, tEntry);
                }
            }
        }
    }

    for (SettledCallback& tCallback : tSettled)
    {
        tCallback(tIsSaved);
    }

    if (tNext)
    {
        send(pCharacterId, std::move(*tNext));
    }
}

void CharacterSaveQueue::armRetry(uint64_t pCharacterId, Entry& pEntry)
{
    pEntry.isWaitingRetry = true;
    ++pEntry.retryToken;

    const uint64_t tToken = pEntry.retryToken;
    const std::chrono::milliseconds tDelay = getRetryDelay(pEntry.attempt);

    boost::asio::post(mContext, [this, pCharacterId, tToken, tDelay]()
    {
        auto tTimer = std::make_shared<boost::asio::steady_timer>(mContext, tDelay);

        tTimer->async_wait([this, pCharacterId, tToken, tTimer](const boost::system::error_code&)
        {
            onRetryTimer(pCharacterId, tToken);
        });
    });
}

void CharacterSaveQueue::onRetryTimer(uint64_t pCharacterId, uint64_t pToken)
{
    std::optional<CharacterRepository::CharacterSave> tToSend;

    {
        std::lock_guard<std::mutex> tLock(mMutex);

        auto tIterator = mEntries.find(pCharacterId);

        if (tIterator == mEntries.end() || tIterator->second.retryToken != pToken || !tIterator->second.isWaitingRetry)
        {
            return;
        }

        Entry& tEntry = tIterator->second;
        tEntry.isWaitingRetry = false;

        if (tEntry.hasUnsent)
        {
            tToSend = takeForSend(tEntry);
        }
    }

    if (tToSend)
    {
        send(pCharacterId, std::move(*tToSend));
    }
}

std::chrono::milliseconds CharacterSaveQueue::getRetryDelay(uint32_t pAttempt)
{
    if (mIsFlushing)
    {
        return kFlushRetryDelay;
    }

    const uint32_t tShift = std::min(pAttempt > 0 ? pAttempt - 1 : 0u, kMaxBackoffShift);
    const std::chrono::milliseconds tBase = std::min(kRetryBaseDelay * (1u << tShift), kRetryMaxDelay);

    std::uniform_real_distribution<double> tJitter(1.0 - kJitterFraction, 1.0 + kJitterFraction);

    return std::chrono::milliseconds(static_cast<int64_t>(static_cast<double>(tBase.count()) * tJitter(mRandom)));
}

void CharacterSaveQueue::logFailure(uint64_t pCharacterId, const Entry& pEntry)
{
    const auto tNow = std::chrono::steady_clock::now();

    if (pEntry.attempt == 1)
    {
        mLastSummaryAt = tNow;

        std::osyncstream(std::cerr) << "[SaveQueue] Karakter kaydi basarisiz, tekrar denenecek (id: " << pCharacterId
                                    << ", sira: " << pEntry.sequence << ")" << std::endl;
        return;
    }

    if (tNow - mLastSummaryAt >= kSummaryInterval)
    {
        mLastSummaryAt = tNow;

        std::osyncstream(std::cerr) << "[SaveQueue] Kaydedilemeyen karakter sayisi: " << mEntries.size() << std::endl;
    }
}

void CharacterSaveQueue::flush(std::chrono::milliseconds pDeadline)
{
    std::vector<std::pair<uint64_t, CharacterRepository::CharacterSave>> tResends;
    std::vector<CharacterRepository::CharacterSave> tUnsaved;

    {
        std::unique_lock<std::mutex> tLock(mMutex);

        if (mIsStopped)
        {
            return;
        }

        mIsFlushing = true;

        for (auto& [tCharacterId, tEntry] : mEntries)
        {
            if (tEntry.isWaitingRetry)
            {
                ++tEntry.retryToken;
                tEntry.isWaitingRetry = false;

                if (tEntry.hasUnsent)
                {
                    tResends.emplace_back(tCharacterId, takeForSend(tEntry));
                }
            }
        }
    }

    for (auto& [tCharacterId, tSave] : tResends)
    {
        send(tCharacterId, std::move(tSave));
    }

    {
        std::unique_lock<std::mutex> tLock(mMutex);

        mErased.wait_for(tLock, pDeadline, [this]()
        {
            return mEntries.empty();
        });

        for (const auto& [tCharacterId, tEntry] : mEntries)
        {
            tUnsaved.push_back(tEntry.latest);
        }

        mIsStopped = true;
    }

    stopContext();
    reportUnsaved(tUnsaved);
}

void CharacterSaveQueue::reportUnsaved(const std::vector<CharacterRepository::CharacterSave>& pUnsaved)
{
    if (pUnsaved.empty())
    {
        return;
    }

    for (const CharacterRepository::CharacterSave& tSave : pUnsaved)
    {
        std::optional<std::filesystem::path> tPath = UnsavedSnapshotWriter::write(tSave);

        std::osyncstream(std::cerr) << "[SaveQueue] HATA: Kaydedilemedi (id: " << tSave.characterId
                                    << ", harita: " << tSave.mapId << ", seviye: " << tSave.stats.level
                                    << ", deneyim: " << tSave.stats.experience << ", altin: " << tSave.stats.gold
                                    << ", esya: " << tSave.items.size()
                                    << ", dosya: " << (tPath ? tPath->string() : std::string("yazilamadi")) << ")" << std::endl;
    }

    std::osyncstream(std::cerr) << "[SaveQueue] HATA: Kapanista kaydedilemeyen karakter sayisi: " << pUnsaved.size() << std::endl;
}
