#include "CommandReply.h"

using namespace lakot;

CommandReply::CommandReply(SessionPtr pSession, uint64_t pRequestId)
    : mSession(std::move(pSession))
    , mRequestId(pRequestId)
{

}

CommandReply::~CommandReply()
{
    if (!mIsSent.load())
    {
        send(services::chat::COMMAND_FAILED, {"Target is no longer online."});
    }
}

void CommandReply::setTarget(std::string pTarget)
{
    mTarget = std::move(pTarget);
}

void CommandReply::setAudit(AuditFunction pAudit)
{
    mAudit = std::move(pAudit);
}

void CommandReply::send(services::chat::CommandStatus pStatus, std::vector<std::string> pLines)
{
    if (mIsSent.exchange(true))
    {
        return;
    }

    std::string tResult = pLines.empty() ? std::string() : pLines.front();

    if (mSession)
    {
        connection::Message tMessage;
        auto* tHeader = tMessage.mutable_response()->mutable_header();
        tHeader->set_reply_to(mRequestId);
        tHeader->mutable_status()->set_code(common::STATUS_OK);

        auto* tResponse = tMessage.mutable_response()->mutable_command_response();
        tResponse->set_status(pStatus);

        for (std::string& tLine : pLines)
        {
            tResponse->add_lines(std::move(tLine));
        }

        mSession->send(tMessage);
    }

    if (mAudit)
    {
        mAudit(mTarget, services::chat::CommandStatus_Name(pStatus) + ": " + tResult);
    }
}

void CommandReply::ok(std::string pLine)
{
    send(services::chat::COMMAND_OK, {std::move(pLine)});
}

void CommandReply::ok(std::vector<std::string> pLines)
{
    send(services::chat::COMMAND_OK, std::move(pLines));
}

void CommandReply::fail(std::string pLine)
{
    send(services::chat::COMMAND_FAILED, {std::move(pLine)});
}

void CommandReply::usage(std::string pLine)
{
    send(services::chat::COMMAND_USAGE, {std::move(pLine)});
}

void CommandReply::complete(bool pIsSuccess, const std::string& pText)
{
    if (pIsSuccess)
    {
        ok(pText);
    }
    else
    {
        fail(pText);
    }
}
