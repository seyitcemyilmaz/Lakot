#ifndef LAKOT_AGARBAGECOLLECTOR_H
#define LAKOT_AGARBAGECOLLECTOR_H

#include <queue>
#include <mutex>

#include <lakot/interface/engine/IGarbageCollector.h>

namespace lakot {

class AGarbageCollector : public IGarbageCollector
{
public:
    virtual ~AGarbageCollector() override;
    AGarbageCollector();

    void add(IObject* pObject) override;

protected:
    std::queue<IObject*> mObjectQueue;

    std::mutex mMutex;
};

}

#endif
