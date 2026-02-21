#ifndef LAKOT_TIMER_H
#define LAKOT_TIMER_H

#include <chrono>
#include <string>

namespace lakot
{

class Timer
{
public:
    virtual ~Timer() = default;
    Timer() = default;

    enum Type
    {
        eNanoseconds,
        eMicroseconds,
        eMilliseconds,
        eSeconds,
        eMinutes,
        eHours
    };

    void start();
    void stop();

    std::string getTime(Type pTimerType = Type::eMilliseconds);

private:
    std::chrono::steady_clock::time_point mStartTime;
    std::chrono::steady_clock::time_point mStopTime;
};

}

#endif
