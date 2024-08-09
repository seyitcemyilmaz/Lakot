#include "GarbageCollector.h"

#include <spdlog/spdlog.h>

using namespace lakot;

GarbageCollector* GarbageCollector::mInstance = nullptr;

GarbageCollector::~GarbageCollector()
{

}

GarbageCollector::GarbageCollector()
    : AGarbageCollector()
{

}

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

void GarbageCollector::process()
{
    const std::lock_guard<std::mutex> tLock(mMutex);

    while (!mObjectQueue.empty())
    {
        IObject* tObject = mObjectQueue.front();

        tObject->deinitialize();
        delete tObject;

        mObjectQueue.pop();
    }
}
