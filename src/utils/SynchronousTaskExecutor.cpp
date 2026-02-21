#include "SynchronousTaskExecutor.h"

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
            // TODO: add handling
        }
        catch (...)
        {
            // TODO: add handling
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
