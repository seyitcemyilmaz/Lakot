#ifndef LAKOT_ARENDERABLE_H
#define LAKOT_ARENDERABLE_H

#include <lakot/interface/graphics/render/IRenderable.h>

namespace lakot {

class ARenderable : public IRenderable
{
public:
    virtual ~ARenderable() override;
    ARenderable();

    RenderableType getType() const override;

protected:
    RenderableType mType;
};

}

#endif
