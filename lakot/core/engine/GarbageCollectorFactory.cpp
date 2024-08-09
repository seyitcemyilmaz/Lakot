#include "GarbageCollectorFactory.h"

using namespace lakot;

GarbageCollector* GarbageCollectorFactory::createGarbageCollector()
{
    return new GarbageCollector();
}
