#ifndef LAKOT_GARBAGECOLLECTOR_H
#define LAKOT_GARBAGECOLLECTOR_H

#include "SynchronousTaskExecutor.h"
#include "AsynchronousTaskExecutor.h"

namespace lakot
{

class GarbageCollector
{
public:
    virtual ~GarbageCollector();
    GarbageCollector();

    void executeSynchronousTasks();

    void addSynchronousTask(std::function<void()>&& pExecuteFunction,
                            std::function<void()>&& pCompletedCallback = nullptr);
    void addSynchronousTask(const Task& pTask);
    void addSynchronousTask(Task&& pTask);

    void addAsynchronousTask(std::function<void()>&& pExecuteFunction,
                             std::function<void()>&& pCompletedCallback = nullptr);
    void addAsynchronousTask(const Task& pTask);
    void addAsynchronousTask(Task&& pTask);

    template <typename T>
    void scheduleDeletion(T* pObject, bool pIsAsynchronous = false)
    {
        if (!pObject)
        {
            return;
        }

        auto tDeleteFunction = [pObject]()
        {
            delete pObject;
        };

        if (pIsAsynchronous)
        {
            addAsynchronousTask(std::move(tDeleteFunction));
        }
        else
        {
            addSynchronousTask(std::move(tDeleteFunction));
        }
    }

private:
    SynchronousTaskExecutor mSynchronousTaskExecutor;
    AsynchronousTaskExecutor mAsynchronousTaskExecutor;
};

}

#endif
