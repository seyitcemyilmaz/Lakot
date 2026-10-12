#include "SynchronousTaskExecutor.h"

#include <iostream>
#include <syncstream>

using namespace lakot;

void SynchronousTaskExecutor::execute()
{
    std::queue<Task> tProcessQueue;

    {
        std::lock_guard<std::mutex> tLock(mTaskMutex);

        tProcessQueue.swap(mTaskQueue);
    }

    while (!tProcessQueue.empty())
    {
        Task& tTask = tProcessQueue.front();

        try
        {
            if (tTask.executeFunction)
            {
                tTask.executeFunction();
            }

            if (tTask.completedCallback)
            {
                tTask.completedCallback();
            }
        }
        catch (const std::exception& tException)
        {
            std::osyncstream(std::cerr) << "[SyncTask] Gorev istisnasi: " << tException.what() << std::endl;
        }
        catch (...)
        {
            std::osyncstream(std::cerr) << "[SyncTask] Gorev bilinmeyen istisna" << std::endl;
        }

        tProcessQueue.pop();
    }
}

void SynchronousTaskExecutor::addTask(std::function<void ()>&& pExecuteFunction,
                                      std::function<void ()>&& pCompletedCallback)
{
    std::lock_guard<std::mutex> tLock(mTaskMutex);

    mTaskQueue.emplace(std::move(pExecuteFunction), std::move(pCompletedCallback));
}

void SynchronousTaskExecutor::addTask(const Task& pTask)
{
    std::lock_guard<std::mutex> tLock(mTaskMutex);

    mTaskQueue.push(pTask);
}

void SynchronousTaskExecutor::addTask(Task&& pTask)
{
    std::lock_guard<std::mutex> tLock(mTaskMutex);

    mTaskQueue.push(std::move(pTask));
}
