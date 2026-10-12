#include "Server.h"
#include "TimerResolution.h"

#include <exception>
#include <iostream>

#if defined(_WIN32)
#include <windows.h>
#include <cstdlib>

namespace
{
    constexpr DWORD kCloseGraceMilliseconds = 4500;

    BOOL WINAPI onConsoleCtrlEvent(DWORD pCtrlType)
    {
        lakot::Server::requestShutdown();

        if (pCtrlType == CTRL_C_EVENT || pCtrlType == CTRL_BREAK_EVENT)
        {
            return TRUE;
        }

        // Windows ends the process once this returns, so wait for main to finish saving.
        Sleep(kCloseGraceMilliseconds);
        std::_Exit(0);
    }
}
#endif

int main()
{
#if defined(_WIN32)
    SetConsoleCtrlHandler(onConsoleCtrlEvent, TRUE);
#endif

    lakot::TimerResolution tTimerResolution;

    try
    {
        lakot::Server tServer;

        if (!tServer.initialize())
        {
            return 1;
        }

        tServer.run();
    }
    catch (const std::exception& tException)
    {
        std::cerr << "[Server] Kritik hata: " << tException.what() << std::endl;
        return 1;
    }

    return 0;
}
