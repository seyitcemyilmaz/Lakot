#include "AsynchronousTaskExecutor.h"

using namespace lakot;

AsynchronousTaskExecutor::~AsynchronousTaskExecutor()
{
    {
        std::lock_guard<std::mutex> tLock(mTaskMutex);
        mIsTaskThreadNeedStop = true;
    }

    mTaskConditionVariable.notify_all();

    if (mTaskThread.joinable())
    {
        mTaskThread.join();
    }
}

AsynchronousTaskExecutor::AsynchronousTaskExecutor()
    : mIsTaskThreadNeedStop(false)
{
    mTaskThread = std::thread([this]() { execute(); });
}

void AsynchronousTaskExecutor::addTask(std::function<void ()>&& pExecuteFunction,
                                       std::function<void ()>&& pCompletedCallback)
{
    {
        std::lock_guard<std::mutex> tLock(mTaskMutex);
        mTaskQueue.emplace(std::move(pExecuteFunction), std::move(pCompletedCallback));
    }

    mTaskConditionVariable.notify_one();
}

void AsynchronousTaskExecutor::addTask(const Task& pTask)
{
    {
        std::lock_guard<std::mutex> tLock(mTaskMutex);
        mTaskQueue.push(pTask);
    }

    mTaskConditionVariable.notify_one();
}

void AsynchronousTaskExecutor::addTask(Task&& pTask)
{
    {
        std::lock_guard<std::mutex> tLock(mTaskMutex);
        mTaskQueue.push(std::move(pTask));
    }

    mTaskConditionVariable.notify_one();
}

void AsynchronousTaskExecutor::execute()
{
    Task tCurrentTask;

    while (getNextTask(tCurrentTask))
    {
        if (tCurrentTask.executeFunction)
        {
            try
            {
                tCurrentTask.executeFunction();
            }
            catch (...)
            {
                // TODO: add handler
            }
        }

        if (tCurrentTask.completedCallback)
        {
            try
            {
                tCurrentTask.completedCallback();
            }
            catch (...)
            {
                // TODO: add handler
            }
        }
    }
}

bool AsynchronousTaskExecutor::getNextTask(Task& pTask)
{
    std::unique_lock<std::mutex> tLock(mTaskMutex);

    mTaskConditionVariable.wait(tLock, [this]()
    {
        return !mTaskQueue.empty() || mIsTaskThreadNeedStop;
    });

    if (mIsTaskThreadNeedStop && mTaskQueue.empty())
    {
        return false;
    }

    pTask = std::move(mTaskQueue.front());
    mTaskQueue.pop();

    return true;
}
