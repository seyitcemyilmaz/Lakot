#ifndef LAKOT_GRAPHICSAPIFACTORY_H
#define LAKOT_GRAPHICSAPIFACTORY_H

#include "GraphicsAPI.h"

namespace lakot {

class GraphicsAPIFactory
{
public:
    virtual ~GraphicsAPIFactory() = default;
    GraphicsAPIFactory() = default;

    static GraphicsAPI* createGraphicsAPI();
};

}

#endif
