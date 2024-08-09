#ifndef LAKOT_IOBJECT_H
#define LAKOT_IOBJECT_H

namespace lakot {

class IObject
{
public:
    virtual ~IObject() = default;
    IObject() = default;

    virtual void initialize() = 0;
    virtual void deinitialize() = 0;
};

}

#endif
