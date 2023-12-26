#ifndef LAKOT_TIMER_H
#define LAKOT_TIMER_H

#include <chrono>

enum TimerType
{
    eNanoseconds,
    eMicroseconds,
    eMilliseconds,
    eSeconds,
    eMinutes,
    eHours
};

class Timer
{
private:
    std::chrono::steady_clock::time_point mStartTime;
    std::chrono::steady_clock::time_point mStopTime;

public:
    virtual ~Timer();
    Timer();

    void start();
    void stop();

    std::string getTime(TimerType pTimerType = TimerType::eMilliseconds);
};

#endif
