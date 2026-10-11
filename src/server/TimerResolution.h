#ifndef LAKOT_SERVER_TIMERRESOLUTION_H
#define LAKOT_SERVER_TIMERRESOLUTION_H

namespace lakot
{

class TimerResolution
{
public:
    TimerResolution();
    ~TimerResolution();

    TimerResolution(const TimerResolution&) = delete;
    TimerResolution& operator=(const TimerResolution&) = delete;
};

}

#endif
