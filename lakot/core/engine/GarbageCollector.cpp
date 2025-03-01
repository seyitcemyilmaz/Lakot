#include "GarbageCollector.h"

#include <spdlog/spdlog.h>

using namespace lakot;

GarbageCollector* GarbageCollector::mInstance = nullptr;

GarbageCollector* GarbageCollector::getInstance()
{
    return mInstance;
}

void GarbageCollector::initialize()
{
    spdlog::info("Garbage collector is initializing.");

    mInstance = this;

    spdlog::info("Garbage collector is initialized.");
}

void GarbageCollector::deinitialize()
{
    spdlog::info("Garbage collector is deinitializing.");

    mInstance = nullptr;

    spdlog::info("Garbage collector is deinitialized.");
}

void GarbageCollector::executeSynchronousProcesses()
{
    mSynchronousProcessExecutor.process();
}

void GarbageCollector::add(const std::function<void ()>& pFunction,
                           bool pIsAsynchronous,
                           const std::function<void ()>& pProcessCompletedCallback)
{
    if (pIsAsynchronous)
    {
        mAsynchronousProcessExecutor.addProcess(pFunction, pProcessCompletedCallback);
    }
    else
    {
        mSynchronousProcessExecutor.addProcess(pFunction, pProcessCompletedCallback);
    }
}
