#ifndef LAKOT_SERVER_BASECONTROLLER_H
#define LAKOT_SERVER_BASECONTROLLER_H

#include <cstdint>

#include <connection.pb.h>

#include "../NetworkManager.h"
#include "../../database/repository/RepositoryManager.h"

namespace lakot
{

class BaseController
{
public:
    virtual ~BaseController() = default;
    explicit BaseController(NetworkManager& pNetworkManager, RepositoryManager& pRepositoryManager)
        : mNetworkManager(pNetworkManager)
        , mRepositoryManager(pRepositoryManager)
    {

    }

    virtual void initialize() = 0;

protected:
    static connection::Message makeResponse(uint64_t pReplyTo, common::StatusCode pStatusCode)
    {
        connection::Message tMessage;
        auto* tHeader = tMessage.mutable_response()->mutable_header();
        tHeader->set_reply_to(pReplyTo);
        tHeader->mutable_status()->set_code(pStatusCode);
        return tMessage;
    }

    NetworkManager& mNetworkManager;
    RepositoryManager& mRepositoryManager;
};

}

#endif
