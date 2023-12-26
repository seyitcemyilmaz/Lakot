#include "Lakot.h"

#include <spdlog/spdlog.h>

#ifdef LAKOT_USE_GPU
extern "C" {
// http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdfG
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;

// http://developer.amd.com/community/blog/2015/10/02/amd-enduro-system-for-developers/
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

Lakot::~Lakot()
{
    delete mApplication;
}

Lakot::Lakot()
{
    mApplication = nullptr;
}

void Lakot::initalizeApplication()
{
    mApplication = new Application();
    mApplication->initialization();
}

void Lakot::runApplication()
{
    spdlog::info("Application is running.");
    mApplication->run();
    spdlog::info("Application is stopped.");
}

void Lakot::terminateApplication()
{
    spdlog::info("Application is terminating.");
    mApplication->terminate();
    spdlog::info("Application is terminated.");
}
