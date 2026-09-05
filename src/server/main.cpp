#include "Server.h"

#if defined(_WIN32)
#include <windows.h>
#include <cstdlib>

namespace
{
    // Without this, closing the console window only sends CTRL_CLOSE_EVENT;
    // Windows gives the process a grace period to react, and defers the
    // decision to terminate entirely to an attached debugger (exactly what
    // happens when this is launched via Qt Creator's Run/Debug) - so the
    // process keeps running until Stop is pressed in the IDE. Exit
    // immediately instead of relying on that default behavior.
    BOOL WINAPI onConsoleCtrlEvent(DWORD pCtrlType)
    {
        switch (pCtrlType)
        {
            case CTRL_CLOSE_EVENT:
            case CTRL_C_EVENT:
            case CTRL_BREAK_EVENT:
            case CTRL_LOGOFF_EVENT:
            case CTRL_SHUTDOWN_EVENT:
            {
                std::_Exit(0);
            }
        }

        return FALSE;
    }
}
#endif

int main()
{
#if defined(_WIN32)
    SetConsoleCtrlHandler(onConsoleCtrlEvent, TRUE);
#endif

    lakot::Server tServer;

    if (tServer.initialize())
    {
        tServer.run();
    }

    return 0;
}
