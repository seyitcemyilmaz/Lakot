#ifndef LAKOT_ILOGGER_H
#define LAKOT_ILOGGER_H

#include "IObject.h"

namespace lakot {

class ILogger : public IObject
{
public:
    virtual ~ILogger() override = default;
    ILogger() = default;
};

}

#endif
