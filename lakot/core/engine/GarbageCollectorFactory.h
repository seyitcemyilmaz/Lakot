#ifndef LAKOT_GARBAGECOLLECTORFACTORY_H
#define LAKOT_GARBAGECOLLECTORFACTORY_H

#include "GarbageCollector.h"

namespace lakot {

class GarbageCollectorFactory
{
public:
    virtual ~GarbageCollectorFactory() = default;
    GarbageCollectorFactory() = default;

    static GarbageCollector* createGarbageCollector();
};

}

#endif
