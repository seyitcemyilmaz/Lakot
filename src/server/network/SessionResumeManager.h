#ifndef LAKOT_SERVER_SESSIONRESUMEMANAGER_H
#define LAKOT_SERVER_SESSIONRESUMEMANAGER_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

namespace lakot
{

// Resume tokens and the grace period after a dropped connection.
//
// One entry per logged-in account. When its session drops, the entry is
// "held" for kHoldDuration: the account and its character stay as they are
// (the character stays in the world), and a new connection presenting the
// token takes the session over. If nobody does, the character is released
// exactly as an immediate disconnect would have released it.
class SessionResumeManager
{
public:
    using ExpiryCallback = std::function<void(uint64_t pAccountId, uint64_t pGeneration)>;

    struct IssueResult
    {
        std::string token;
        uint64_t releasedCharacterId = 0;
    };

    struct ResumeResult
    {
        uint64_t accountId = 0;
        uint64_t characterId = 0;
        std::string token;
    };

    static constexpr std::chrono::seconds kHoldDuration{45};

    SessionResumeManager(boost::asio::io_context& pContext, ExpiryCallback pExpiryCallback);

    IssueResult issueToken(uint64_t pAccountId);

    void setCharacter(uint64_t pAccountId, uint64_t pCharacterId);

    uint64_t revoke(uint64_t pAccountId);

    uint64_t expireHold(uint64_t pAccountId, uint64_t pGeneration);

    // The account's session dropped. True if it is now held and will be
    // released later; false if there is nothing to hold (the caller releases
    // immediately).
    bool hold(uint64_t pAccountId);

    // Takes over a held (or still attached, e.g. half-open) session.
    // Consumes the token and returns the next one.
    std::optional<ResumeResult> resume(const std::string& pToken);

private:
    struct Entry
    {
        uint64_t characterId = 0;
        std::string token;
        bool isHeld = false;
        uint64_t holdGeneration = 0;
        std::shared_ptr<boost::asio::steady_timer> timer;
    };

    boost::asio::io_context& mContext;

    ExpiryCallback mExpiryCallback;

    std::mutex mMutex;
    std::unordered_map<uint64_t, Entry> mEntries;           // accountId -> entry
    std::unordered_map<std::string, uint64_t> mTokens;      // token -> accountId

    // Removes the entry; returns the character it still had, 0 if none.
    uint64_t eraseLocked(uint64_t pAccountId);

    static std::string generateToken();
};

}

#endif
