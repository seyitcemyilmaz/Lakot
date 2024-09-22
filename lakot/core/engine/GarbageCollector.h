#ifndef LAKOT_GARBAGECOLLECTOR_H
#define LAKOT_GARBAGECOLLECTOR_H

#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

#include <lakot/interface/engine/IObject.h>

namespace lakot {

class GarbageCollector
{
public:
    static GarbageCollector* getInstance();

    void initialize();
    void deinitialize();

    void synchronousProcess();

    template<typename T>
    typename std::enable_if<!std::is_base_of<IObject, T>::value>::type
    add(T* pObject, bool pIsAsynchronous = false)
    {
        std::function<void()> tFunction = [pObject]()
        {
            delete pObject;
        };

        add(tFunction, pIsAsynchronous);
    }

    template<typename T>
    typename std::enable_if<std::is_base_of<IObject, T>::value>::type
    add(T* pObject, bool pIsAsynchronous = false)
    {
        std::function<void()> tFunction = [pObject]()
        {
            pObject->deinitialize();
            delete pObject;
        };

        add(tFunction, pIsAsynchronous);
    }

    void add(const std::function<void ()>& pFunction, bool pIsAsynchronous = false);

protected:
    static GarbageCollector* mInstance;
    friend class GarbageCollectorFactory;

private:
    // Asynchronous part
    std::thread mAsynchronousThread;
    std::atomic<bool> mIsAsynchronousThreadNeedStop;

    std::mutex mAsynchronousMutex;
    std::queue<std::function<void()>> mAsynchronousQueue;

    std::condition_variable mAsynchronousConditionVariable;

    // Synchronous part
    std::mutex mSynchronousMutex;
    std::queue<std::function<void()>> mSynchronousQueue;

    void asynchronousProcess();

    virtual ~GarbageCollector();
    GarbageCollector();
};

}

#endif
