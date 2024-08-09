#ifndef LAKOT_LAYERFACTORY_H
#define LAKOT_LAYERFACTORY_H

#include "Layer.h"

namespace lakot {

class LayerFactory
{
public:
    virtual ~LayerFactory() = default;
    LayerFactory() = default;

    static Layer* createLayer();
};

}

#endif
