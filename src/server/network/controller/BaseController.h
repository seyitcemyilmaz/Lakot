#ifndef LAKOT_SERVER_BASECONTROLLER_H
#define LAKOT_SERVER_BASECONTROLLER_H

#include "../NetworkManager.h"

namespace lakot
{

class BaseController
{
public:
    virtual ~BaseController() = default;
    explicit BaseController(NetworkManager& pNetworkManager)
        : mNetworkManager(pNetworkManager)
    {

    }

    virtual void initialize() = 0;

protected:
    NetworkManager& mNetworkManager;
};

}

#endif
