#include <spdlog/spdlog.h>

#include <lakot/core/engine/EngineFactory.h>

#ifdef LAKOT_USE_GPU
extern "C"
{
// http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdfG
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;

// http://developer.amd.com/community/blog/2015/10/02/amd-enduro-system-for-developers/
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

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
