#ifndef LAKOT_OBJECT_H
#define LAKOT_OBJECT_H

namespace lakot {

class Object
{
public:
    virtual ~Object() = default;
    Object() = default;

    virtual void initialize() = 0;
    virtual void deinitialize() = 0;
};

}


#endif
