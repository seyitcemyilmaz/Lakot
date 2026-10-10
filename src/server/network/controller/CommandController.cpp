#include "CommandController.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <syncstream>

#include "WorldController.h"
#include "../RequestLimits.h"

using namespace lakot;

CommandController::CommandController(NetworkManager& pNetworkManager, RepositoryManager& pRepositoryManager, WorldController& pWorldController)
    : BaseController(pNetworkManager, pRepositoryManager)
    , mWorldController(pWorldController)
{

}

void CommandController::initialize()
{
    registerCommand({"help", {}, 0, 0, 1, "/help [command]", "List the commands you can use.",
        [this](const CommandContext& pContext, std::shared_ptr<CommandReply> pReply)
        {
            this->executeHelp(pContext, std::move(pReply));
        }});

    registerCommand({"who", {}, 0, 0, 0, "/who", "Show how many players are online.",
        [this](const CommandContext& pContext, std::shared_ptr<CommandReply> pReply)
        {
            this->executeWho(pContext, std::move(pReply));
        }});

    registerCommand({"pos", {}, 0, 0, 0, "/pos", "Show your map and position.",
        [this](const CommandContext& pContext, std::shared_ptr<CommandReply> pReply)
        {
            this->executePosition(pContext, std::move(pReply));
        }});

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kCommandRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleCommandRequest(pSession, pMessage);
        }
    );
}

bool CommandController::registerCommand(CommandSpec pSpec)
{
    std::vector<std::string> tNames = pSpec.aliases;
    tNames.push_back(pSpec.name);

    for (const std::string& tName : tNames)
    {
        if (mIndex.contains(tName))
        {
            std::osyncstream(std::cerr) << "[Command] Komut adi zaten kayitli: " << tName << std::endl;
            return false;
        }
    }

    size_t tPosition = mCommands.size();
    mCommands.push_back(std::move(pSpec));

    for (const std::string& tName : tNames)
    {
        mIndex.emplace(tName, tPosition);
    }

    return true;
}

const CommandController::CommandSpec* CommandController::find(const std::string& pName) const
{
    auto tIterator = mIndex.find(pName);
    return tIterator == mIndex.end() ? nullptr : &mCommands[tIterator->second];
}

void CommandController::handleCommandRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tCharacterId = pSession->getCharacterId();

    if (tCharacterId == 0)
    {
        return;
    }

    const std::string& tText = pMessage.request().command_request().text();
    auto tReply = std::make_shared<CommandReply>(pSession, pMessage.request().header().id());

    auto tAccess = mWorldController.getCharacterAccess(tCharacterId);

    if (!tAccess)
    {
        tReply->fail("You are not in the world.");
        return;
    }

    CommandContext tContext;
    const CommandSpec* tSpec = nullptr;

    if (RequestLimits::isValidChatText(tText) && CommandContext::parse(tText, tContext) == CommandParseType::eParsed)
    {
        tSpec = find(tContext.name);
    }

    bool tIsAuthorized = tSpec && tAccess->gmLevel >= tSpec->minLevel;
    bool tIsGmCommand = tIsAuthorized && tSpec->minLevel >= 1;
    TokenBucketLimiter& tLimiter = tIsGmCommand ? mGmLimiter : mLimiter;

    if (!tLimiter.tryConsume(tCharacterId))
    {
        tReply->send(services::chat::COMMAND_THROTTLED, {"You are sending commands too quickly."});
        return;
    }

    if (!tIsAuthorized)
    {
        if (tSpec)
        {
            std::osyncstream(std::cout) << "[Security] Yetkisiz komut denemesi: karakter " << tCharacterId
                                        << " hesap " << tAccess->accountId << " /" << tContext.name << std::endl;
        }

        tReply->send(services::chat::COMMAND_UNKNOWN, {kUnknownCommandText});
        return;
    }

    tContext.characterId = tCharacterId;
    tContext.accountId = tAccess->accountId;
    tContext.characterName = mWorldController.getCharacterName(tCharacterId);
    tContext.gmLevel = tAccess->gmLevel;

    if (tIsGmCommand)
    {
        attachAudit(*tSpec, tContext, *tReply);
    }

    if (tContext.getArgumentCount() < tSpec->minArguments || tContext.getArgumentCount() > tSpec->maxArguments)
    {
        tReply->usage("Usage: " + tSpec->usage);
        return;
    }

    try
    {
        tSpec->handler(tContext, tReply);
    }
    catch (const std::exception& tException)
    {
        std::osyncstream(std::cerr) << "[Command] /" << tContext.name << " istisna: " << tException.what() << std::endl;
        tReply->fail("Command failed.");
    }
}

void CommandController::attachAudit(const CommandSpec& pSpec, const CommandContext& pContext, CommandReply& pReply)
{
    GmLogRepository::Entry tEntry;
    tEntry.accountId = pContext.accountId;
    tEntry.characterId = pContext.characterId;
    tEntry.characterName = pContext.characterName;
    tEntry.gmLevel = pContext.gmLevel;
    tEntry.command = pSpec.name;
    tEntry.arguments = pContext.argumentText.substr(0, kMaxAuditArgumentLength);

    std::osyncstream(std::cout) << "[GM] hesap " << tEntry.accountId << " karakter " << tEntry.characterId
                                << " (" << tEntry.characterName << ") seviye " << tEntry.gmLevel
                                << " /" << tEntry.command << " " << tEntry.arguments << std::endl;

    GmLogRepository& tRepository = mRepositoryManager.getGmLogRepository();

    pReply.setAudit(
    [&tRepository, tEntry](const std::string& pTarget, const std::string& pResult)
    {
        GmLogRepository::Entry tCompleted = tEntry;
        tCompleted.target = pTarget;
        tCompleted.result = pResult.substr(0, kMaxAuditArgumentLength);

        std::osyncstream(std::cout) << "[GM] sonuc: karakter " << tCompleted.characterId << " /" << tCompleted.command
                                    << " hedef '" << tCompleted.target << "' -> " << tCompleted.result << std::endl;

        tRepository.record(std::move(tCompleted));
    });
}

void CommandController::executeHelp(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply)
{
    if (pContext.getArgumentCount() == 1)
    {
        std::string tName(pContext.getArgument(0));

        if (!tName.empty() && tName.front() == '/')
        {
            tName.erase(0, 1);
        }

        std::transform(tName.begin(), tName.end(), tName.begin(),
                       [](unsigned char pCharacter) { return static_cast<char>(std::tolower(pCharacter)); });

        const CommandSpec* tSpec = find(tName);

        if (!tSpec || tSpec->minLevel > pContext.gmLevel)
        {
            pReply->send(services::chat::COMMAND_UNKNOWN, {kUnknownCommandText});
            return;
        }

        pReply->ok(std::vector<std::string>{"Usage: " + tSpec->usage, tSpec->summary});
        return;
    }

    std::vector<const CommandSpec*> tVisible;

    for (const CommandSpec& tSpec : mCommands)
    {
        if (tSpec.minLevel <= pContext.gmLevel)
        {
            tVisible.push_back(&tSpec);
        }
    }

    std::sort(tVisible.begin(), tVisible.end(),
              [](const CommandSpec* pLeft, const CommandSpec* pRight)
              {
                  return pLeft->minLevel != pRight->minLevel ? pLeft->minLevel < pRight->minLevel : pLeft->name < pRight->name;
              });

    std::vector<std::string> tLines;
    tLines.push_back("Available commands:");

    for (const CommandSpec* tSpec : tVisible)
    {
        tLines.push_back(tSpec->usage + " - " + tSpec->summary);
    }

    pReply->ok(std::move(tLines));
}

void CommandController::executeWho(const CommandContext&, std::shared_ptr<CommandReply> pReply)
{
    pReply->ok(std::to_string(mWorldController.getOnlineCount()) + " player(s) online.");
}

void CommandController::executePosition(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply)
{
    Zone* tZone = mWorldController.findCharacterZone(pContext.characterId);

    if (!tZone)
    {
        pReply->fail("You are not in the world.");
        return;
    }

    tZone->queryPosition(pContext.characterId,
    [pReply](bool pIsFound, uint32_t pMapId, float pX, float pZ)
    {
        if (!pIsFound)
        {
            pReply->fail("You are not in the world.");
            return;
        }

        char tBuffer[96];
        std::snprintf(tBuffer, sizeof(tBuffer), "Map %u, x %.1f, z %.1f.", pMapId, pX, pZ);
        pReply->ok(std::string(tBuffer));
    });
}
