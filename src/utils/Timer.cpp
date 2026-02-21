#include "Timer.h"

using namespace lakot;

void Timer::start()
{
    mStartTime = std::chrono::steady_clock::now();
}

void Timer::stop()
{
    mStopTime = std::chrono::steady_clock::now();
}

std::string Timer::getTime(Type pTimerType)
{
    if (pTimerType == Type::eNanoseconds)
    {
        return std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(mStopTime - mStartTime).count()) + "ns";
    }

    if (pTimerType == Type::eMicroseconds)
    {
        return std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(mStopTime - mStartTime).count()) + "µs";
    }

    if (pTimerType == Type::eMilliseconds)
    {
        return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(mStopTime - mStartTime).count()) + "ms";
    }

    if (pTimerType == Type::eSeconds)
    {
        return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(mStopTime - mStartTime).count()) + "s";
    }

    if (pTimerType == Type::eMinutes)
    {
        return std::to_string(std::chrono::duration_cast<std::chrono::minutes>(mStopTime - mStartTime).count()) + "m";
    }

    if (pTimerType == Type::eHours)
    {
        return std::to_string(std::chrono::duration_cast<std::chrono::hours>(mStopTime - mStartTime).count()) + "h";
    }

    throw "Undefined behaviour";
}
