#ifndef LAKOT_THREADSAFEQUEUE_HPP
#define LAKOT_THREADSAFEQUEUE_HPP

#include <mutex>
#include <deque>
#include <optional>

namespace lakot
{

template<typename T>
class ThreadSafeQueue
{
public:
    ~ThreadSafeQueue()
    {
        clear();
    }

    ThreadSafeQueue() = default;
    ThreadSafeQueue(const ThreadSafeQueue<T>&) = delete;

    void push_back(const T& pItem)
    {
        std::scoped_lock tLock(mMutex);
        mQueue.push_back(pItem);
    }

    void push_back(T&& pItem)
    {
        std::scoped_lock tLock(mMutex);
        mQueue.emplace_back(std::move(pItem));
    }

    void push_front(const T& pItem)
    {
        std::scoped_lock tLock(mMutex);
        mQueue.push_front(pItem);
    }

    void push_front(T&& pItem)
    {
        std::scoped_lock tLock(mMutex);
        mQueue.emplace_front(std::move(pItem));
    }

    bool isEmpty() const
    {
        std::scoped_lock tLock(mMutex);
        return mQueue.empty();
    }

    size_t size() const
    {
        std::scoped_lock tLock(mMutex);
        return mQueue.size();
    }

    void clear()
    {
        std::scoped_lock tLock(mMutex);
        mQueue.clear();
    }

    std::optional<T> pop_front()
    {
        std::scoped_lock tLock(mMutex);

        if (mQueue.empty())
        {
            return std::nullopt;
        }

        T tItem = std::move(mQueue.front());
        mQueue.pop_front();
        return tItem;
    }

protected:
    mutable std::mutex mMutex;
    std::deque<T> mQueue;
};

}

#endif
