#ifndef LAKOT_GARBAGECOLLECTOR_H
#define LAKOT_GARBAGECOLLECTOR_H

#include <lakot/utilities/SynchronousProcessExecutor.h>
#include <lakot/utilities/AsynchronousProcessExecutor.h>

#include <lakot/utilities/Object.h>

namespace lakot {

class GarbageCollector
{
public:
    static GarbageCollector* getInstance();

    void initialize();
    void deinitialize();

    void executeSynchronousProcesses();

    template<typename T>
    typename std::enable_if<!std::is_base_of<Object, T>::value>::type
    add(T* pObject,
        bool pIsAsynchronous = false,
        const std::function<void ()>& pProcessCompletedCallback = nullptr)
    {
        std::function<void()> tFunction = [pObject]()
        {
            delete pObject;
        };

        add(tFunction, pIsAsynchronous, pProcessCompletedCallback);
    }

    template<typename T>
    typename std::enable_if<std::is_base_of<Object, T>::value>::type
    add(T* pObject,
        bool pIsAsynchronous = false,
        const std::function<void ()>& pProcessCompletedCallback = nullptr)
    {
        std::function<void()> tFunction = [pObject]()
        {
            pObject->deinitialize();
            delete pObject;
        };

        add(tFunction, pIsAsynchronous, pProcessCompletedCallback);
    }

protected:
    static GarbageCollector* mInstance;
    friend class GarbageCollectorFactory;

private:
    SynchronousProcessExecutor mSynchronousProcessExecutor;
    AsynchronousProcessExecutor mAsynchronousProcessExecutor;

    void add(const std::function<void ()>& pFunction,
             bool pIsAsynchronous = false,
             const std::function<void ()>& pProcessCompletedCallback = nullptr);

    virtual ~GarbageCollector() = default;
    GarbageCollector() = default;
};

}

#endif
