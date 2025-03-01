#ifndef LAKOT_SYNCHRONOUSPROCESSEXECUTOR_H
#define LAKOT_SYNCHRONOUSPROCESSEXECUTOR_H

#include <mutex>
#include <queue>
#include <functional>

#include "Type.h"

namespace lakot {

class SynchronousProcessExecutor
{
public:
    virtual ~SynchronousProcessExecutor() = default;
    SynchronousProcessExecutor() = default;

    void process();

    void addProcess(const std::function<void ()>& pExecuteFunction, const std::function<void ()>& pProcessCompletedCallback = nullptr);
    void addProcess(const Process& pProcess);
    void addProcess(Process&& pProcess);

private:
    std::mutex mProcessMutex;
    std::queue<Process> mProcessQueue;
};

}

#endif
