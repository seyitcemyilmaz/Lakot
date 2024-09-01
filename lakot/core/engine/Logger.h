#ifndef LAKOT_LOGGER_H
#define LAKOT_LOGGER_H

#include <lakot/abstract/engine/ALogger.h>

namespace lakot {

class Logger : public ALogger
{
public:
    static Logger* getInstance();

    void initialize() override;
    void deinitialize() override;

protected:
    virtual ~Logger() override;
    Logger();

    static Logger* mInstance;
};

}

#endif
