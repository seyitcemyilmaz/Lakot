#ifndef LAKOT_ASYNCHRONOUSPROCESSEXECUTOR_H
#define LAKOT_ASYNCHRONOUSPROCESSEXECUTOR_H

#include <mutex>
#include <queue>
#include <thread>
#include <functional>

#include "Type.h"

namespace lakot {

class AsynchronousProcessExecutor
{
public:
    virtual ~AsynchronousProcessExecutor();
    AsynchronousProcessExecutor();

    void addProcess(const std::function<void ()>& pExecuteFunction, const std::function<void ()>& pProcessCompletedCallback = nullptr);
    void addProcess(const Process& pProcess);
    void addProcess(Process&& pProcess);

private:
    std::thread mProcessThread;
    std::atomic<bool> mIsProcessThreadNeedStop;

    std::mutex mProcessMutex;
    std::queue<Process> mProcessQueue;

    std::condition_variable mProcessConditionVariable;

    void process();
};

}

#endif
