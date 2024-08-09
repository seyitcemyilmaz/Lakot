#include "EngineFactory.h"

using namespace lakot;

Engine* EngineFactory::createEngine()
{
    return new Engine();
}
