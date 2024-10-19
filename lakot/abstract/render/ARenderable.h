#ifndef LAKOT_ARENDERABLE_H
#define LAKOT_ARENDERABLE_H

#include <lakot/utilities/Object.h>
#include <lakot/utilities/Type.h>

namespace lakot {

class ARenderable : public Object
{
public:
    virtual ~ARenderable() override;
    ARenderable();

    RenderableType getType() const;

protected:
    RenderableType mType;
};

}

#endif
