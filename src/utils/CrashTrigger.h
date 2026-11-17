#ifndef LAKOT_CRASHTRIGGER_H
#define LAKOT_CRASHTRIGGER_H

#include <string_view>

namespace lakot {

class CrashTrigger
{
public:
    static void runIfRequested();

private:
    CrashTrigger() = delete;
    ~CrashTrigger() = delete;

    static void run(std::string_view pKind);

    static void triggerNull();
    static void triggerDivideByZero();
    static void triggerStackOverflow();
    static void triggerThrow();
    static void triggerTerminate();
    static void triggerAbort();
    static void triggerAssert();
    static void triggerPureCall();
    static void triggerInvalidParameter();
    static void triggerCaughtError();
};

}

#endif
