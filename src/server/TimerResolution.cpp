#include "TimerResolution.h"

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#endif

using namespace lakot;

namespace
{

constexpr unsigned int kPeriodMs = 1;

}

TimerResolution::TimerResolution()
{
#ifdef _WIN32
    timeBeginPeriod(kPeriodMs);
#endif
}

TimerResolution::~TimerResolution()
{
#ifdef _WIN32
    timeEndPeriod(kPeriodMs);
#endif
}
