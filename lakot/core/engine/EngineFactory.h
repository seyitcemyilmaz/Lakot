#ifndef LAKOT_ENGINEFACTORY_H
#define LAKOT_ENGINEFACTORY_H

#include "Engine.h"

namespace lakot {

class EngineFactory
{
public:
    virtual ~EngineFactory() = default;
    EngineFactory() = default;

    static Engine* createEngine();
};

}

#endif
