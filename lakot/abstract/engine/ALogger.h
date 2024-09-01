#ifndef ALOGGER_H
#define ALOGGER_H

#include <lakot/interface/engine/ILogger.h>

namespace lakot {

class ALogger : public ILogger
{
public:
    virtual ~ALogger() override;
    ALogger();
};

}

#endif
