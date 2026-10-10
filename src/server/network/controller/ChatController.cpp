#include "ChatController.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <string>

#include "CommandController.h"
#include "WorldController.h"
#include "../RequestLimits.h"

using namespace lakot;

bool ChatController::consumeChatBudget(uint64_t pSenderId, double pCost)
{
    auto tNow = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> tLock(mBudgetMutex);

    ChatBudget& tBudget = mBudgets[pSenderId];

    if (tBudget.lastRefill.time_since_epoch().count() == 0)
    {
        tBudget.lastRefill = tNow;
    }

    double tElapsedSeconds = std::chrono::duration<double>(tNow - tBudget.lastRefill).count();
    tBudget.lastRefill = tNow;

    tBudget.tokens = std::min(kChatBucketCapacity, tBudget.tokens + tElapsedSeconds * kChatTokensPerSecond);

    if (tBudget.tokens < pCost)
    {
        return false;
    }

    tBudget.tokens -= pCost;
    return true;
}

bool ChatController::consumeGlobalCooldown(uint64_t pSenderId, int& pRemainingSeconds)
{
    auto tNow = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> tLock(mBudgetMutex);

    ChatBudget& tBudget = mBudgets[pSenderId];

    if (tBudget.lastGlobalAt.time_since_epoch().count() != 0)
    {
        double tElapsedSeconds = std::chrono::duration<double>(tNow - tBudget.lastGlobalAt).count();

        if (tElapsedSeconds < kGlobalCooldownSeconds)
        {
            // Rounded up, so "1s" never means "any moment now, actually 1.9s".
            pRemainingSeconds = static_cast<int>(std::ceil(kGlobalCooldownSeconds - tElapsedSeconds));
            return false;
        }
    }

    tBudget.lastGlobalAt = tNow;
    pRemainingSeconds = 0;
    return true;
}

ChatController::ChatController(NetworkManager& pNetworkManager, RepositoryManager& pRepositoryManager,
                               WorldController& pWorldController, CommandController& pCommandController)
    : BaseController(pNetworkManager, pRepositoryManager)
    , mWorldController(pWorldController)
    , mCommandController(pCommandController)
{

}

std::string ChatController::getMuteNotice(uint64_t pCharacterId)
{
    auto tAccess = mWorldController.getCharacterAccess(pCharacterId);

    if (!tAccess)
    {
        return {};
    }

    int64_t tRemainingSeconds = tAccess->getMuteRemainingSeconds();

    if (tRemainingSeconds <= 0)
    {
        return {};
    }

    return "You are muted for " + std::to_string((tRemainingSeconds + 59) / 60) + " more minute(s).";
}

ChatController::WhisperResultType ChatController::deliverWhisper(uint64_t pSenderId, const std::string& pTargetName, const std::string& pText)
{
    if (!getMuteNotice(pSenderId).empty())
    {
        return WhisperResultType::eMuted;
    }

    if (!consumeChatBudget(pSenderId, kWhisperCost))
    {
        return WhisperResultType::eThrottled;
    }

    auto tTargetId = mWorldController.findOnlineCharacterIdByName(pTargetName);
    auto tTargetSession = tTargetId ? mNetworkManager.getSessionRegistry().getByCharacter(*tTargetId) : nullptr;

    if (!tTargetSession)
    {
        return WhisperResultType::eOffline;
    }

    connection::Message tPushMessage;
    auto* tReceived = tPushMessage.mutable_response()->mutable_direct_message_received();
    tReceived->set_from_player_id(pSenderId);
    tReceived->set_from_username(mWorldController.getCharacterName(pSenderId));
    tReceived->set_text(pText);

    tTargetSession->send(tPushMessage);

    return WhisperResultType::eDelivered;
}

void ChatController::executeWhisper(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply)
{
    std::string tTargetName(pContext.getArgument(0));
    std::string tText = pContext.getRest(1);

    if (tTargetName.size() > RequestLimits::kMaxUsernameLength)
    {
        pReply->fail("Player is not online.");
        return;
    }

    switch (deliverWhisper(pContext.characterId, tTargetName, tText))
    {
        case WhisperResultType::eDelivered:
            pReply->ok("To " + tTargetName + ": " + tText);
            break;
        case WhisperResultType::eThrottled:
            pReply->send(services::chat::COMMAND_THROTTLED, {"You are sending messages too quickly."});
            break;
        case WhisperResultType::eMuted:
            pReply->fail(getMuteNotice(pContext.characterId));
            break;
        case WhisperResultType::eOffline:
            pReply->fail("Player is not online.");
            break;
    }
}

void ChatController::executeRoll(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply)
{
    int64_t tMaximum = 100;

    if (pContext.getArgumentCount() >= 1)
    {
        auto tValue = pContext.getInteger(0, 2, 1000);

        if (!tValue)
        {
            pReply->usage("Usage: /roll [2-1000]");
            return;
        }

        tMaximum = *tValue;
    }

    std::string tMuteNotice = getMuteNotice(pContext.characterId);

    if (!tMuteNotice.empty())
    {
        pReply->fail(tMuteNotice);
        return;
    }

    thread_local std::mt19937_64 tRandom{std::random_device{}()};
    int64_t tRoll = std::uniform_int_distribution<int64_t>(1, tMaximum)(tRandom);

    connection::Message tMessage;
    auto* tSystem = tMessage.mutable_response()->mutable_system_message();
    tSystem->set_kind(services::chat::SYSTEM_MESSAGE_ROLL);
    tSystem->set_text(pContext.characterName + " rolls " + std::to_string(tRoll) + " (1-" + std::to_string(tMaximum) + ").");

    auto tPacket = NetworkSession<connection::Message>::makePacket(tMessage);

    if (auto tSession = mNetworkManager.getSessionRegistry().getByCharacter(pContext.characterId))
    {
        tSession->send(tPacket);
    }

    mWorldController.sendNearby(pContext.characterId, tPacket);

    pReply->send(services::chat::COMMAND_OK, {});
}

void ChatController::initialize()
{
    mCommandController.registerCommand({"w", {"whisper", "msg"}, 0, 2, CommandContext::kMaxArguments, "/w <player> <message>", "Whisper to a player.",
        [this](const CommandContext& pContext, std::shared_ptr<CommandReply> pReply)
        {
            this->executeWhisper(pContext, std::move(pReply));
        }});

    mCommandController.registerCommand({"roll", {}, 0, 0, 1, "/roll [2-1000]", "Roll a random number, shown to nearby players.",
        [this](const CommandContext& pContext, std::shared_ptr<CommandReply> pReply)
        {
            this->executeRoll(pContext, std::move(pReply));
        }});

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kDirectMessageRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleDirectMessageRequest(pSession, pMessage);
        }
    );

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kChatMessageRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleChatMessageRequest(pSession, pMessage);
        }
    );
}

void ChatController::handleDirectMessageRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tSenderId = pSession->getCharacterId();

    if (tSenderId == 0)
    {
        return; // authenticated but not in the world yet (character select)
    }

    const auto& tRequest = pMessage.request().direct_message_request();

    // Validated before any lookup or delivery. Without this a single request
    // could carry megabytes of text (the only previous ceiling was the 10 MB
    // frame cap) straight through to another player's chat window.
    if (!RequestLimits::isValidChatText(tRequest.text())
        || tRequest.message_to().size() > RequestLimits::kMaxUsernameLength)
    {
        connection::Message tInvalidMessage;
        auto* tInvalidHeader = tInvalidMessage.mutable_response()->mutable_header();
        tInvalidHeader->set_reply_to(pMessage.request().header().id());
        tInvalidHeader->mutable_status()->set_code(common::STATUS_INVALID_REQUEST);
        tInvalidMessage.mutable_response()->mutable_direct_message_response()
            ->set_error_code(services::chat::DIRECT_MESSAGE_ERROR);

        pSession->send(tInvalidMessage);
        return;
    }

    connection::Message tAckMessage;
    auto* tAckHeader = tAckMessage.mutable_response()->mutable_header();
    tAckHeader->set_reply_to(pMessage.request().header().id());
    tAckHeader->mutable_status()->set_code(common::STATUS_OK);
    auto* tAck = tAckMessage.mutable_response()->mutable_direct_message_response();

    switch (deliverWhisper(tSenderId, tRequest.message_to(), tRequest.text()))
    {
        case WhisperResultType::eDelivered:
            tAck->set_error_code(services::chat::DIRECT_MESSAGE_OK);
            break;
        case WhisperResultType::eOffline:
            tAck->set_error_code(services::chat::DIRECT_MESSAGE_DISCONNECTED);
            break;
        case WhisperResultType::eThrottled:
            tAckHeader->mutable_status()->set_code(common::STATUS_INVALID_REQUEST);
            tAckHeader->mutable_status()->set_message("You are sending messages too quickly.");
            tAck->set_error_code(services::chat::DIRECT_MESSAGE_ERROR);
            break;
        case WhisperResultType::eMuted:
            tAckHeader->mutable_status()->set_code(common::STATUS_INVALID_REQUEST);
            tAckHeader->mutable_status()->set_message(getMuteNotice(tSenderId));
            tAck->set_error_code(services::chat::DIRECT_MESSAGE_ERROR);
            break;
    }

    pSession->send(tAckMessage);
}

void ChatController::handleChatMessageRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tSenderId = pSession->getCharacterId();

    if (tSenderId == 0)
    {
        return; // authenticated but not in the world yet (character select)
    }

    const auto& tRequest = pMessage.request().chat_message_request();

    // The client has already fully decided this (scope button, or an
    // explicit leading "!" for one message regardless of the button - see
    // WorldChatRmlController::doSend()) and sent clean text with no marker
    // left in it - the server just trusts the flag, no text-sniffing here.
    bool tIsGlobal = tRequest.is_global();

    // Same reasoning as the whisper path, but the stakes are higher: a world
    // message is fanned out to an entire area of interest, and a global one to
    // every session on the server, so an unbounded or spammed message here is
    // an amplification vector rather than just an oversized packet.
    if (!RequestLimits::isValidChatText(tRequest.text()) || tRequest.text().front() == '/')
    {
        connection::Message tInvalidMessage;
        auto* tInvalidHeader = tInvalidMessage.mutable_response()->mutable_header();
        tInvalidHeader->set_reply_to(pMessage.request().header().id());
        tInvalidHeader->mutable_status()->set_code(common::STATUS_INVALID_REQUEST);
        tInvalidMessage.mutable_response()->mutable_chat_message_response()
            ->set_error_code(services::chat::CHAT_MESSAGE_ERROR);

        pSession->send(tInvalidMessage);
        return;
    }

    // Two different limits for two different costs: a flat cooldown for
    // server-wide messages, the shared conversational bucket for nearby ones.
    std::string tThrottleMessage = getMuteNotice(tSenderId);

    if (tThrottleMessage.empty() && tIsGlobal)
    {
        int tRemainingSeconds = 0;

        if (!consumeGlobalCooldown(tSenderId, tRemainingSeconds))
        {
            tThrottleMessage = "You can send another world message in "
                             + std::to_string(tRemainingSeconds) + "s.";
        }
    }
    else if (tThrottleMessage.empty() && !consumeChatBudget(tSenderId, kNearbyMessageCost))
    {
        tThrottleMessage = "You are sending messages too quickly.";
    }

    if (!tThrottleMessage.empty())
    {
        connection::Message tThrottledMessage;
        auto* tThrottledHeader = tThrottledMessage.mutable_response()->mutable_header();
        tThrottledHeader->set_reply_to(pMessage.request().header().id());
        tThrottledHeader->mutable_status()->set_code(common::STATUS_INVALID_REQUEST);
        tThrottledHeader->mutable_status()->set_message(tThrottleMessage);
        tThrottledMessage.mutable_response()->mutable_chat_message_response()
            ->set_error_code(services::chat::CHAT_MESSAGE_ERROR);

        pSession->send(tThrottledMessage);
        return;
    }

    connection::Message tPushMessage;
    auto* tReceived = tPushMessage.mutable_response()->mutable_chat_message_received();
    tReceived->set_from_player_id(tSenderId);
    tReceived->set_from_username(mWorldController.getCharacterName(tSenderId));
    tReceived->set_text(tRequest.text());
    tReceived->set_is_global(tIsGlobal);

    // Serialized once here rather than once per recipient inside every
    // send() - a global message on a busy server goes to every session, and
    // re-encoding the same bytes N times is the single most wasteful thing
    // the broadcast path used to do.
    auto tPushPacket = NetworkSession<connection::Message>::makePacket(tPushMessage);

    if (tIsGlobal)
    {
        // True server-wide, regardless of map - every character currently in
        // the world, not just this map's zone. Players still sitting on the
        // character selection screen are deliberately not included: they have
        // no character in the world to receive it as.
        for (uint64_t tOtherId : mNetworkManager.getSessionRegistry().getAllCharacterIds())
        {
            if (tOtherId == tSenderId)
            {
                continue;
            }

            if (auto tOtherSession = mNetworkManager.getSessionRegistry().getByCharacter(tOtherId))
            {
                tOtherSession->send(tPushPacket);
            }
        }
    }
    else
    {
        // "Who is nearby" is zone-thread state now, so this hands the packet
        // to the sender's zone and lets it fan out on its own thread rather
        // than reading a visibility set from this I/O thread.
        mWorldController.sendNearby(tSenderId, tPushPacket);
    }

    connection::Message tAckMessage;
    auto* tAckHeader = tAckMessage.mutable_response()->mutable_header();
    tAckHeader->set_reply_to(pMessage.request().header().id());
    tAckHeader->mutable_status()->set_code(common::STATUS_OK);
    tAckMessage.mutable_response()->mutable_chat_message_response()->set_error_code(services::chat::CHAT_MESSAGE_OK);

    pSession->send(tAckMessage);
}
