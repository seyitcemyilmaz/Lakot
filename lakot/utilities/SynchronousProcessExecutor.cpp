#include "SynchronousProcessExecutor.h"

using namespace lakot;

void SynchronousProcessExecutor::process()
{
    const std::lock_guard<std::mutex> tLock(mProcessMutex);

    while (!mProcessQueue.empty())
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

void SynchronousProcessExecutor::addProcess(const std::function<void ()>& pExecuteFunction, const std::function<void ()>& pProcessCompletedCallback)
{
    std::lock_guard<std::mutex> tLock(mProcessMutex);

    mProcessQueue.push({pExecuteFunction, pProcessCompletedCallback});
}

void SynchronousProcessExecutor::addProcess(const Process& pProcess)
{
    std::lock_guard<std::mutex> tLock(mProcessMutex);

    mProcessQueue.push(pProcess);
}

void SynchronousProcessExecutor::addProcess(Process&& pProcess)
{
    std::lock_guard<std::mutex> tLock(mProcessMutex);

    mProcessQueue.push(std::move(pProcess));
}
