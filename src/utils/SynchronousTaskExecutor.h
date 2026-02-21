#ifndef LAKOT_SYNCHRONOUSTASKEXECUTOR_H
#define LAKOT_SYNCHRONOUSTASKEXECUTOR_H

#include <mutex>
#include <queue>

#include "Task.hpp"

namespace lakot
{

class SynchronousTaskExecutor
{
public:
    virtual ~SynchronousTaskExecutor() = default;
    SynchronousTaskExecutor() = default;

    void execute();

    void addTask(std::function<void ()>&& pExecuteFunction,
                 std::function<void ()>&& pCompletedCallback = nullptr);
    void addTask(const Task& pTask);
    void addTask(Task&& pTask);

private:
    std::mutex mTaskMutex;
    std::queue<Task> mTaskQueue;
};

}

#endif
