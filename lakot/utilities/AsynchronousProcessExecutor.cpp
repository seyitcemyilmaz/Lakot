#include "AsynchronousProcessExecutor.h"

#include <spdlog/spdlog.h>

using namespace lakot;

AsynchronousProcessExecutor::~AsynchronousProcessExecutor()
{
    mIsProcessThreadNeedStop = true;
    mProcessConditionVariable.notify_all();

    if (mProcessThread.joinable())
    {
        mProcessThread.join();
    }
}

AsynchronousProcessExecutor::AsynchronousProcessExecutor()
    : mIsProcessThreadNeedStop(false)
{
    mProcessThread = std::thread([this]() { process(); });
    mProcessThread.detach();
}

void AsynchronousProcessExecutor::addProcess(const std::function<void ()>& pExecuteFunction, const std::function<void ()>& pProcessCompletedCallback)
{
    std::lock_guard<std::mutex> tLock(mProcessMutex);

    mProcessQueue.push(std::move(Process(pExecuteFunction, pProcessCompletedCallback)));
    mProcessConditionVariable.notify_one();
}

void AsynchronousProcessExecutor::addProcess(const Process& pProcess)
{
    std::lock_guard<std::mutex> tLock(mProcessMutex);

    mProcessQueue.push(pProcess);
    mProcessConditionVariable.notify_one();
}

void AsynchronousProcessExecutor::addProcess(Process&& pProcess)
{
    std::lock_guard<std::mutex> tLock(mProcessMutex);

    mProcessQueue.push(std::move(pProcess));
    mProcessConditionVariable.notify_one();
}

void AsynchronousProcessExecutor::process()
{
    while (!mIsProcessThreadNeedStop)
    {
        std::unique_lock<std::mutex> tLock(mProcessMutex);

        mProcessConditionVariable.wait(
            tLock,
            [this]
            {
                return !mProcessQueue.empty() || mIsProcessThreadNeedStop;
            }
        );

        if (!mProcessQueue.empty())
        {
            const Process& tProcess = mProcessQueue.front();

            const std::function<void()>& tExecuteFunction = tProcess.executeFunction;

            if (!tExecuteFunction)
            {
                throw std::invalid_argument("Process execute function cannot be null.");
            }

            tExecuteFunction();

            const std::function<void()>& tProcessCompletedCallback = tProcess.processCompletedCallback;

            if (tProcessCompletedCallback)
            {
                tProcessCompletedCallback();
            }

            mProcessQueue.pop();
        }
    }
}
