#ifndef LAKOT_ARENDERER_H
#define LAKOT_ARENDERER_H

#include "ARenderable.h"

namespace lakot {

class ARenderer : public Object
{
public:
    virtual ~ARenderer() override;
    ARenderer();

    virtual void render(ARenderable* pRenderable) = 0;
};

}

#endif
