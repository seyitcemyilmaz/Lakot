#include <spdlog/spdlog.h>

#include <lakot/core/engine/EngineFactory.h>

int main()
{
    try
    {
        lakot::Engine* tEngine = lakot::EngineFactory::createEngine();
        tEngine->initialize();
        tEngine->run();
        tEngine->deinitialize();
    }
    catch(const char* tError)
    {
        spdlog::error("Exception: {0}", tError);
    }

    return 0;
}
