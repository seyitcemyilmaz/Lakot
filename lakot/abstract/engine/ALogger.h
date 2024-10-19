#ifndef ALOGGER_H
#define ALOGGER_H

#include <lakot/utilities/Object.h>
#include <lakot/utilities/Type.h>

namespace lakot {

class ALogger : public Object
{
public:
    virtual ~ALogger() override;
    ALogger();
};

}

#endif
