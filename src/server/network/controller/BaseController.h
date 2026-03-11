#ifndef LAKOT_SERVER_BASECONTROLLER_H
#define LAKOT_SERVER_BASECONTROLLER_H

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
    NetworkManager& mNetworkManager;
    RepositoryManager& mRepositoryManager;
};

}

#endif
