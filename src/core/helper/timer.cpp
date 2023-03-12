#include "timer.h"

void Timer::start() {
    mStartTime = std::chrono::steady_clock::now();
}

void Timer::stop() {
    mStopTime = std::chrono::steady_clock::now();
}

std::string Timer::getTime(TimerType pTimerType) {
    if (pTimerType == TimerType::eNanoseconds) {
        return std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(mStopTime - mStartTime).count()) + "ns";
    }

    if (pTimerType == TimerType::eMicroseconds) {
        return std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(mStopTime - mStartTime).count()) + "µs";
    }

    if (pTimerType == TimerType::eMilliseconds) {
        return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(mStopTime - mStartTime).count()) + "ms";
    }

    if (pTimerType == TimerType::eSeconds) {
        return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(mStopTime - mStartTime).count()) + "s";
    }

    if (pTimerType == TimerType::eMinutes) {
        return std::to_string(std::chrono::duration_cast<std::chrono::minutes>(mStopTime - mStartTime).count()) + "m";
    }

    if (pTimerType == TimerType::eMinutes) {
        return std::to_string(std::chrono::duration_cast<std::chrono::hours>(mStopTime - mStartTime).count()) + "h";
    }

    throw "Undefined behaviour";
}
