#include "GarbageCollector.h"

#include <spdlog/spdlog.h>

using namespace lakot;

GarbageCollector* GarbageCollector::mInstance = nullptr;

GarbageCollector::~GarbageCollector()
{

}

GarbageCollector::GarbageCollector()
    : mIsAsynchronousThreadNeedStop(false)
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

    mAsynchronousThread = std::thread([this]() { asynchronousProcess(); });

    spdlog::info("Garbage collector is initialized.");
}

void GarbageCollector::deinitialize()
{
    spdlog::info("Garbage collector is deinitializing.");

    mInstance = nullptr;

    mIsAsynchronousThreadNeedStop = true;
    mAsynchronousConditionVariable.notify_all();

    if (mAsynchronousThread.joinable())
    {
        mAsynchronousThread.join();
    }

    spdlog::info("Garbage collector is deinitialized.");
}

void GarbageCollector::synchronousProcess()
{
    const std::lock_guard<std::mutex> tLock(mSynchronousMutex);

    while (!mSynchronousQueue.empty())
    {
        std::function<void()> tExecuteSynchronousFunction = mSynchronousQueue.front();

        tExecuteSynchronousFunction();

        mSynchronousQueue.pop();
    }
}

void GarbageCollector::add(const std::function<void ()>& pFunction, bool pIsAsynchronous)
{
    if (pIsAsynchronous)
    {
        std::lock_guard<std::mutex> tLock(mAsynchronousMutex);

        mAsynchronousQueue.push(pFunction);

        mAsynchronousConditionVariable.notify_one();
    }
    else
    {
        std::lock_guard<std::mutex> tLock(mSynchronousMutex);

        mSynchronousQueue.push(pFunction);
    }
}

void GarbageCollector::asynchronousProcess()
{
    while (!mIsAsynchronousThreadNeedStop)
    {
        std::unique_lock<std::mutex> tLock(mAsynchronousMutex);

        mAsynchronousConditionVariable.wait(tLock,
            [this]
            {
                return !mAsynchronousQueue.empty() || mIsAsynchronousThreadNeedStop;
            }
        );

        if (!mAsynchronousQueue.empty())
        {
            auto tDeleteFunction = std::move(mAsynchronousQueue.front());
            mAsynchronousQueue.pop();
            tLock.unlock();
            tDeleteFunction();
        }
    }
}
