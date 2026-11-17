#include "diagnostics/CrashHandler.h"

#include "diagnostics/LogRing.h"
#include "diagnostics/LogTee.h"
#include "diagnostics/ThreadName.h"

#include <csignal>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>

#include <execinfo.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

using namespace lakot;

namespace
{
    constexpr std::size_t kAlternateStackSize = 64 * 1024;
    constexpr std::size_t kLogCopySize = 32 * 1024;
    constexpr int kMaxBacktraceFrames = 64;

    char sExecutableName[64];
    char sBaseDirectory[512];
    char sAlternateStack[kAlternateStackSize];
    char sLogCopy[kLogCopySize];
    LogRing* sRing = nullptr;
    thread_local char* tAlternateStack = nullptr;

    std::size_t appendText(char* pOut, std::size_t pPosition, std::size_t pCapacity, const char* pText)
    {
        while (*pText && pPosition + 1 < pCapacity)
        {
            pOut[pPosition++] = *pText++;
        }

        pOut[pPosition] = '\0';
        return pPosition;
    }

    std::size_t appendNumber(char* pOut, std::size_t pPosition, std::size_t pCapacity, unsigned long long pValue, unsigned pBase)
    {
        char tDigits[24];
        std::size_t tCount = 0;

        do
        {
            tDigits[tCount++] = "0123456789abcdef"[pValue % pBase];
            pValue /= pBase;
        } while (pValue > 0 && tCount < sizeof(tDigits));

        while (tCount > 0 && pPosition + 1 < pCapacity)
        {
            pOut[pPosition++] = tDigits[--tCount];
        }

        pOut[pPosition] = '\0';
        return pPosition;
    }

    void writeAll(int pDescriptor, const char* pText, std::size_t pLength)
    {
        while (pLength > 0)
        {
            const ssize_t tWritten = write(pDescriptor, pText, pLength);

            if (tWritten <= 0)
            {
                return;
            }

            pText += tWritten;
            pLength -= static_cast<std::size_t>(tWritten);
        }
    }

    const char* kindOf(int pSignal)
    {
        switch (pSignal)
        {
        case SIGSEGV:
            return "sigsegv";
        case SIGBUS:
            return "sigbus";
        case SIGFPE:
            return "sigfpe";
        case SIGILL:
            return "sigill";
        default:
            return "sigabrt";
        }
    }

    void onSignal(int pSignal, siginfo_t* pInfo, void*)
    {
        char tLine[512];
        std::size_t tPosition = 0;
        tPosition = appendText(tLine, tPosition, sizeof(tLine), "CRASH ");
        tPosition = appendText(tLine, tPosition, sizeof(tLine), sExecutableName);
        tPosition = appendText(tLine, tPosition, sizeof(tLine), " ");
        tPosition = appendText(tLine, tPosition, sizeof(tLine), kindOf(pSignal));
        tPosition = appendText(tLine, tPosition, sizeof(tLine), " fault 0x");
        tPosition = appendNumber(tLine, tPosition, sizeof(tLine), pInfo ? reinterpret_cast<unsigned long long>(pInfo->si_addr) : 0, 16);
        tPosition = appendText(tLine, tPosition, sizeof(tLine), " at ?:0 in ? (thread ");
        tPosition = appendText(tLine, tPosition, sizeof(tLine), ThreadName::get());
        tPosition = appendText(tLine, tPosition, sizeof(tLine), ":");
        tPosition = appendNumber(tLine, tPosition, sizeof(tLine), static_cast<unsigned long long>(syscall(SYS_gettid)), 10);
        tPosition = appendText(tLine, tPosition, sizeof(tLine), ")\n");

        char tDirectory[640];
        std::size_t tLength = appendText(tDirectory, 0, sizeof(tDirectory), sBaseDirectory);
        mkdir(tDirectory, 0755);
        tLength = appendText(tDirectory, tLength, sizeof(tDirectory), "/");
        tLength = appendText(tDirectory, tLength, sizeof(tDirectory), sExecutableName);
        tLength = appendText(tDirectory, tLength, sizeof(tDirectory), "-");
        tLength = appendNumber(tDirectory, tLength, sizeof(tDirectory), static_cast<unsigned long long>(time(nullptr)), 10);
        tLength = appendText(tDirectory, tLength, sizeof(tDirectory), "-");
        tLength = appendNumber(tDirectory, tLength, sizeof(tDirectory), static_cast<unsigned long long>(getpid()), 10);
        mkdir(tDirectory, 0755);

        char tPath[700];
        appendText(tPath, appendText(tPath, 0, sizeof(tPath), tDirectory), sizeof(tPath), "/summary.txt");
        int tFile = open(tPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (tFile >= 0)
        {
            writeAll(tFile, tLine, tPosition);
            close(tFile);
        }

        appendText(tPath, appendText(tPath, 0, sizeof(tPath), tDirectory), sizeof(tPath), "/report.txt");
        tFile = open(tPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (tFile >= 0)
        {
            void* tFrames[kMaxBacktraceFrames];
            const int tCount = backtrace(tFrames, kMaxBacktraceFrames);
            writeAll(tFile, tLine, tPosition);
            writeAll(tFile, "== stack ==\n", 12);
            backtrace_symbols_fd(tFrames, tCount, tFile);
            writeAll(tFile, "== log ==\n", 10);

            if (sRing)
            {
                const std::size_t tLogLength = sRing->copyTail(sLogCopy, sizeof(sLogCopy), 200, 0);
                writeAll(tFile, sLogCopy, tLogLength);
            }

            close(tFile);
        }

        writeAll(STDERR_FILENO, tLine, tPosition);

        std::signal(pSignal, SIG_DFL);
        raise(pSignal);
    }

    void onTerminate()
    {
        std::abort();
    }

    void installAlternateStack(char* pStack, std::size_t pSize)
    {
        stack_t tStack = {};
        tStack.ss_sp = pStack;
        tStack.ss_size = pSize;
        sigaltstack(&tStack, nullptr);
    }
}

void CrashHandler::install(std::string_view pExecutableName)
{
    std::snprintf(sExecutableName, sizeof(sExecutableName), "%.*s", static_cast<int>(pExecutableName.size()), pExecutableName.data());

    const char* tBase = std::getenv("LAKOT_CRASH_DIR");
    const char* tTestDirectory = std::getenv("LAKOT_TEST_LOG_DIR");

    if (tBase && tBase[0])
    {
        std::snprintf(sBaseDirectory, sizeof(sBaseDirectory), "%s", tBase);
    }
    else if (tTestDirectory && tTestDirectory[0])
    {
        std::snprintf(sBaseDirectory, sizeof(sBaseDirectory), "%s/crashes", tTestDirectory);
    }
    else
    {
        std::snprintf(sBaseDirectory, sizeof(sBaseDirectory), "crashes");
    }

    // Leaked on purpose: std::cout may still write during static destruction.
    sRing = new LogRing;
    std::cout.rdbuf(new LogTee(*std::cout.rdbuf(), *sRing));
    std::cerr.rdbuf(new LogTee(*std::cerr.rdbuf(), *sRing));
    std::clog.rdbuf(new LogTee(*std::clog.rdbuf(), *sRing));

    void* tPrime[4];
    backtrace(tPrime, 4);

    installAlternateStack(sAlternateStack, sizeof(sAlternateStack));

    struct sigaction tAction = {};
    tAction.sa_sigaction = onSignal;
    tAction.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND | SA_NODEFER;
    sigemptyset(&tAction.sa_mask);

    for (int tSignal : {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT})
    {
        sigaction(tSignal, &tAction, nullptr);
    }

    std::set_terminate(onTerminate);
    ThreadName::set("main");
}

void CrashHandler::attachThread(std::string_view pThreadName)
{
    ThreadName::set(pThreadName);

    if (!tAlternateStack)
    {
        tAlternateStack = new char[kAlternateStackSize];
        installAlternateStack(tAlternateStack, kAlternateStackSize);
    }
}

void CrashHandler::recordLog(std::string_view pText)
{
    if (sRing)
    {
        sRing->append(pText);
    }
}

const char* CrashHandler::getExecutableName()
{
    return sExecutableName[0] ? sExecutableName : "unknown";
}
