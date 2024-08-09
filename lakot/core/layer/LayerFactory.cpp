#include "LayerFactory.h"

using namespace lakot;

Layer* LayerFactory::createLayer()
{
    return new Layer();
}
