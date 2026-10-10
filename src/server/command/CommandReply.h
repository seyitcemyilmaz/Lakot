#ifndef LAKOT_SERVER_COMMANDREPLY_H
#define LAKOT_SERVER_COMMANDREPLY_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <connection.pb.h>

#include "NetworkSession.h"

namespace lakot
{

class CommandReply
{
public:
    using SessionPtr = std::shared_ptr<NetworkSession<connection::Message>>;
    using AuditFunction = std::function<void(const std::string& pTarget, const std::string& pResult)>;

    CommandReply(SessionPtr pSession, uint64_t pRequestId);
    ~CommandReply();

    CommandReply(const CommandReply&) = delete;
    CommandReply& operator=(const CommandReply&) = delete;

    void setTarget(std::string pTarget);
    void setAudit(AuditFunction pAudit);

    void send(services::chat::CommandStatus pStatus, std::vector<std::string> pLines);
    void ok(std::string pLine);
    void ok(std::vector<std::string> pLines);
    void fail(std::string pLine);
    void usage(std::string pLine);
    void complete(bool pIsSuccess, const std::string& pText);

private:
    SessionPtr mSession;
    uint64_t mRequestId;
    AuditFunction mAudit;
    std::string mTarget;
    std::atomic<bool> mIsSent{false};
};

}

#endif
