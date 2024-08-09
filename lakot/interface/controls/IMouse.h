#ifndef LAKOT_IMOUSE_H
#define LAKOT_IMOUSE_H

#include "../engine/IObject.h"

namespace lakot {

class IMouse : public IObject
{
public:
    virtual ~IMouse() = default;
    IMouse() = default;

    virtual double getX() const = 0;
    virtual double getY() const = 0;

    virtual double getDeltaX() const = 0;
    virtual double getDeltaY() const = 0;

    virtual double getScrollDeltaX() const = 0;
    virtual double getScrollDeltaY() const = 0;
};

}

#endif
