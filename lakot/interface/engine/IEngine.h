#ifndef LAKOT_IENGINE_H
#define LAKOT_IENGINE_H

#include "IObject.h"

namespace lakot {

class IEngine : virtual public IObject
{
public:
    virtual ~IEngine() override = default;
    IEngine() = default;

    virtual void run() = 0;
};

}

#endif
