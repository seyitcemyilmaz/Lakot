#ifndef LAKOT_IENGINE_H
#define LAKOT_IENGINE_H

#include "IObject.h"

namespace lakot {

class IEngine : public IObject
{
public:
    virtual ~IEngine() = default;
    IEngine() = default;

    virtual void run() = 0;
};

}

#endif
