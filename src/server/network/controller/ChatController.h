#ifndef LAKOT_SERVER_CHATCONTROLLER_H
#define LAKOT_SERVER_CHATCONTROLLER_H

#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>

#include <connection.pb.h>

#include "BaseController.h"

namespace lakot
{

template <typename MessageType>
class NetworkSession;

class WorldController;

class ChatController : public BaseController
{
public:
    ChatController(NetworkManager& pNetworkManager, RepositoryManager& pRepositoryManager, WorldController& pWorldController);

    void initialize() override;


private:
    // Token bucket per sender for the conversational channels (nearby world
    // chat and whispers): a burst of a few messages is fine, that is how
    // people actually talk, but the sustained rate is capped.
    static constexpr double kChatBucketCapacity = 5.0;
    static constexpr double kChatTokensPerSecond = 1.0;
    static constexpr double kNearbyMessageCost = 1.0;
    static constexpr double kWhisperCost = 1.0;

    // Global ("!") messages are NOT rationed by the bucket. They reach every
    // player on the server, so the limit that matters is a flat minimum gap
    // between them rather than a burst allowance - and a burst allowance is
    // exactly the wrong shape here: charging a global message several tokens
    // let two go out back to back and then blocked the third, which read as
    // an arbitrary failure ("the same message worked a second ago"). One
    // fixed cooldown is both stricter about actual spam and predictable.
    static constexpr double kGlobalCooldownSeconds = 15.0;

    struct ChatBudget
    {
        double tokens = kChatBucketCapacity;
        std::chrono::steady_clock::time_point lastRefill{};

        // Zero-initialized means "never sent one", which the cooldown check
        // treats as allowed.
        std::chrono::steady_clock::time_point lastGlobalAt{};
    };

    WorldController& mWorldController;

    // Dispatch runs on the network worker pool, so this is reached from
    // several threads at once.
    std::mutex mBudgetMutex;
    std::unordered_map<uint64_t, ChatBudget> mBudgets;

    // Deducts pCost if the sender can afford it. False means the message is
    // dropped as spam.
    bool consumeChatBudget(uint64_t pSenderId, double pCost);

    // Consumes the global-message cooldown. Returns true if a global message
    // may be sent now; otherwise returns false and writes the whole seconds
    // still to wait into pRemainingSeconds, so the player is told how long
    // rather than just "too fast".
    bool consumeGlobalCooldown(uint64_t pSenderId, int& pRemainingSeconds);

    void handleDirectMessageRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleChatMessageRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
};

}

#endif
