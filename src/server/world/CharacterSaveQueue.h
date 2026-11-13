#ifndef LAKOT_SERVER_CHARACTERSAVEQUEUE_H
#define LAKOT_SERVER_CHARACTERSAVEQUEUE_H

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>

#include <boost/asio.hpp>

#include "../database/repository/CharacterRepository.h"

namespace lakot
{

class CharacterSaveQueue
{
public:
    using SettledCallback = std::function<void(bool pIsSaved)>;

    explicit CharacterSaveQueue(CharacterRepository& pRepository);
    ~CharacterSaveQueue();

    CharacterSaveQueue(const CharacterSaveQueue&) = delete;
    CharacterSaveQueue& operator=(const CharacterSaveQueue&) = delete;

    void submit(CharacterRepository::CharacterSave pSave);

    // pCallback may run on a database worker thread: it must not block.
    void whenSettled(uint64_t pCharacterId, SettledCallback pCallback);

    void flush(std::chrono::milliseconds pDeadline);

    size_t getUnsavedCount() const;

private:
    struct Entry
    {
        CharacterRepository::CharacterSave latest;
        bool hasUnsent = false;
        bool isInFlight = false;
        bool isWaitingRetry = false;
        uint32_t attempt = 0;
        uint32_t permanentFailures = 0;
        uint64_t retryToken = 0;
        uint64_t sequence = 0;
        std::vector<SettledCallback> waiters;
    };

    CharacterRepository& mRepository;

    mutable std::mutex mMutex;
    std::condition_variable mErased;
    std::unordered_map<uint64_t, Entry> mEntries;
    bool mIsFlushing = false;
    bool mIsStopped = false;
    std::mt19937 mRandom;
    std::chrono::steady_clock::time_point mLastSummaryAt;

    boost::asio::io_context mContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> mWorkGuard;
    std::thread mThread;

    static CharacterRepository::CharacterSave takeForSend(Entry& pEntry);
    void send(uint64_t pCharacterId, CharacterRepository::CharacterSave pSave);
    void onSaveResult(uint64_t pCharacterId, CharacterRepository::SaveResult pResult);
    void onRetryTimer(uint64_t pCharacterId, uint64_t pToken);
    void armRetry(uint64_t pCharacterId, Entry& pEntry);
    std::chrono::milliseconds getRetryDelay(uint32_t pAttempt);
    void logFailure(uint64_t pCharacterId, const Entry& pEntry);
    void reportUnsaved(const std::vector<CharacterRepository::CharacterSave>& pUnsaved);
    void stopContext();
};

}

#endif
