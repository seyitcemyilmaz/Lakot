#ifndef LAKOT_TASK_HPP
#define LAKOT_TASK_HPP

#include <functional>

namespace lakot
{

struct Task
{
    Task() = default;

    Task(std::function<void()>&& pExecuteFunction,
         std::function<void()>&& pCompletedCallback = nullptr)
        : executeFunction(std::move(pExecuteFunction))
        , completedCallback(std::move(pCompletedCallback))
    {

    }

    std::function<void ()> executeFunction;
    std::function<void ()> completedCallback;
};

}

#endif
