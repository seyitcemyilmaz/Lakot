#ifndef LAKOT_IGARBAGECOLLECTOR_H
#define LAKOT_IGARBAGECOLLECTOR_H

#include "IObject.h"

namespace lakot {

class IGarbageCollector : public IObject
{
public:
    virtual ~IGarbageCollector() = default;
    IGarbageCollector() = default;

    virtual void process() = 0;

    virtual void add(IObject* pObject) = 0;
};

}

#endif
