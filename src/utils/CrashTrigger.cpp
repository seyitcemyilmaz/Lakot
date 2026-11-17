#include "CrashTrigger.h"

#include "diagnostics/CrashHandler.h"
#include "diagnostics/ErrorReport.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

using namespace lakot;

namespace
{
    volatile bool sKeepRecursing = true;

    int recurse(int pDepth)
    {
        volatile char tPadding[1024];
        tPadding[0] = static_cast<char>(pDepth);

        if (sKeepRecursing)
        {
            return recurse(pDepth + 1) + tPadding[0];
        }

        return tPadding[0];
    }

    class PureBase
    {
    public:
        PureBase()
        {
            PureBase* volatile tSelf = this;
            tSelf->callPure();
        }

        virtual ~PureBase() = default;

        virtual void callPure() = 0;
    };

    class PureDerived : public PureBase
    {
    public:
        void callPure() override
        {
        }
    };
}

void CrashTrigger::runIfRequested()
{
    const char* tRequest = std::getenv("LAKOT_CRASH_TEST");

    if (!tRequest || !tRequest[0])
    {
        return;
    }

    const std::string_view tValue(tRequest);
    const std::size_t tAt = tValue.find('@');
    const std::string_view tKind = tValue.substr(0, tAt);

    if (tAt == std::string_view::npos)
    {
        run(tKind);
        return;
    }

    std::thread tThread([tKind]()
    {
        CrashHandler::attachThread("crash-test");
        run(tKind);
    });

    tThread.join();
}

void CrashTrigger::run(std::string_view pKind)
{
    if (pKind == "null")
    {
        triggerNull();
    }
    else if (pKind == "divzero")
    {
        triggerDivideByZero();
    }
    else if (pKind == "stackoverflow")
    {
        triggerStackOverflow();
    }
    else if (pKind == "throw")
    {
        triggerThrow();
    }
    else if (pKind == "terminate")
    {
        triggerTerminate();
    }
    else if (pKind == "abort")
    {
        triggerAbort();
    }
    else if (pKind == "assert")
    {
        triggerAssert();
    }
    else if (pKind == "purecall")
    {
        triggerPureCall();
    }
    else if (pKind == "invalidparam")
    {
        triggerInvalidParameter();
    }
    else if (pKind == "error")
    {
        triggerCaughtError();
    }
    else
    {
        std::cerr << "[Diag] Unknown LAKOT_CRASH_TEST value: " << pKind << std::endl;
    }
}

void CrashTrigger::triggerNull()
{
    volatile int* volatile tTarget = nullptr;
    *tTarget = 1;
}

void CrashTrigger::triggerDivideByZero()
{
    volatile int tDivisor = 0;
    volatile int tResult = 10 / tDivisor;
    (void)tResult;
}

void CrashTrigger::triggerStackOverflow()
{
    volatile int tResult = recurse(0);
    (void)tResult;
}

void CrashTrigger::triggerThrow()
{
    throw std::runtime_error("LAKOT_CRASH_TEST throw");
}

void CrashTrigger::triggerTerminate()
{
    std::terminate();
}

void CrashTrigger::triggerAbort()
{
    std::abort();
}

void CrashTrigger::triggerAssert()
{
#if defined(NDEBUG)
    std::cerr << "[Diag] assert is compiled out in this build, continuing" << std::endl;
#else
    assert(sKeepRecursing && false && "LAKOT_CRASH_TEST assert");
#endif
}

void CrashTrigger::triggerPureCall()
{
    PureDerived tObject;
    (void)tObject;
}

void CrashTrigger::triggerInvalidParameter()
{
#if defined(_WIN32)
    char tSmall[2];
    strcpy_s(tSmall, sizeof(tSmall), "LAKOT_CRASH_TEST");
#else
    std::cerr << "[Diag] invalidparam is only available on Windows" << std::endl;
#endif
}

void CrashTrigger::triggerCaughtError()
{
    try
    {
        throw std::out_of_range("LAKOT_CRASH_TEST error");
    }
    catch (const std::exception& tException)
    {
        ErrorReport::report("crash-test", tException);
    }
}
