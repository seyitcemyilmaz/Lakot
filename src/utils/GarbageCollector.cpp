#include "GarbageCollector.h"

using namespace lakot;

GarbageCollector::~GarbageCollector()
{

}

GarbageCollector::GarbageCollector()
{

}

void GarbageCollector::executeSynchronousTasks()
{
    mSynchronousTaskExecutor.execute();
}

void GarbageCollector::addSynchronousTask(std::function<void()>&& pExecuteFunction,
                                          std::function<void()>&& pCompletedCallback)
{
    mSynchronousTaskExecutor.addTask(std::move(pExecuteFunction),
                                     std::move(pCompletedCallback));
}

void GarbageCollector::addSynchronousTask(const Task& pTask)
{
    mSynchronousTaskExecutor.addTask(pTask);
}

void GarbageCollector::addSynchronousTask(Task&& pTask)
{
    mSynchronousTaskExecutor.addTask(std::move(pTask));
}

void GarbageCollector::addAsynchronousTask(std::function<void()>&& pExecuteFunction,
                                           std::function<void()>&& pCompletedCallback)
{
    mAsynchronousTaskExecutor.addTask(std::move(pExecuteFunction),
                                      std::move(pCompletedCallback));
}

void GarbageCollector::addAsynchronousTask(const Task& pTask)
{
    mAsynchronousTaskExecutor.addTask(pTask);
}

void GarbageCollector::addAsynchronousTask(Task&& pTask)
{
    mAsynchronousTaskExecutor.addTask(std::move(pTask));
}
