#ifndef LAKOT_ASYNCHRONOUSTASKEXECUTOR_H
#define LAKOT_ASYNCHRONOUSTASKEXECUTOR_H

#include <mutex>
#include <queue>

#include "Task.hpp"

namespace lakot
{

class AsynchronousTaskExecutor
{
public:
    virtual ~AsynchronousTaskExecutor();
    AsynchronousTaskExecutor();

    void addTask(std::function<void ()>&& pExecuteFunction,
                 std::function<void ()>&& pCompletedCallback = nullptr);
    void addTask(const Task& pTask);
    void addTask(Task&& pTask);

private:
    std::thread mTaskThread;
    std::atomic<bool> mIsTaskThreadNeedStop;

    std::mutex mTaskMutex;
    std::queue<Task> mTaskQueue;

    std::condition_variable mTaskConditionVariable;

    void execute();
    bool getNextTask(Task& pTask);
};

}

#endif
