#ifndef LAKOT_SERVER_COMMANDCONTROLLER_H
#define LAKOT_SERVER_COMMANDCONTROLLER_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <connection.pb.h>

#include "BaseController.h"

#include "../../command/CommandContext.h"
#include "../../command/CommandReply.h"
#include "../../security/TokenBucketLimiter.h"

namespace lakot
{

template <typename MessageType>
class NetworkSession;

class WorldController;

class CommandController : public BaseController
{
public:
    using Handler = std::function<void(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply)>;

    struct CommandSpec
    {
        std::string name;
        std::vector<std::string> aliases;
        uint32_t minLevel = 0;
        size_t minArguments = 0;
        size_t maxArguments = 0;
        std::string usage;
        std::string summary;
        Handler handler;
    };

    static constexpr const char* kUnknownCommandText = "Unknown command. Type /help.";

    CommandController(NetworkManager& pNetworkManager, RepositoryManager& pRepositoryManager, WorldController& pWorldController);

    void initialize() override;

    bool registerCommand(CommandSpec pSpec);

private:
    static constexpr double kBucketCapacity = 5.0;
    static constexpr double kBucketTokensPerSecond = 2.0;
    static constexpr double kGmBucketCapacity = 20.0;
    static constexpr double kGmBucketTokensPerSecond = 5.0;
    static constexpr double kDeniedLogBucketCapacity = 1.0;
    static constexpr double kDeniedLogTokensPerSecond = 1.0 / 60.0;
    static constexpr size_t kMaxAuditArgumentLength = 256;

    WorldController& mWorldController;

    TokenBucketLimiter mLimiter{kBucketCapacity, kBucketTokensPerSecond};
    TokenBucketLimiter mGmLimiter{kGmBucketCapacity, kGmBucketTokensPerSecond};
    TokenBucketLimiter mDeniedLogLimiter{kDeniedLogBucketCapacity, kDeniedLogTokensPerSecond};

    std::vector<CommandSpec> mCommands;
    std::unordered_map<std::string, size_t> mIndex;

    void handleCommandRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);

    const CommandSpec* find(const std::string& pName) const;
    void attachAudit(const CommandSpec& pSpec, const CommandContext& pContext, CommandReply& pReply);

    void executeHelp(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply);
    void executeWho(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply);
    void executePosition(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply);
};

}

#endif
