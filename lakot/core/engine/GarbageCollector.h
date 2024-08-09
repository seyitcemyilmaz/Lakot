#ifndef LAKOT_GARBAGECOLLECTOR_H
#define LAKOT_GARBAGECOLLECTOR_H

#include <lakot/abstract/engine/AGarbageCollector.h>

namespace lakot {

class GarbageCollector : public AGarbageCollector
{
public:
    virtual ~GarbageCollector() override;
    GarbageCollector();

    static GarbageCollector* getInstance();

    void initialize() override;
    void deinitialize() override;

    void process() override;

protected:
    static GarbageCollector* mInstance;
};

}

#endif
