#include "diagnostics/CrashHandler.h"

#include "diagnostics/CrashClassifier.h"
#include "diagnostics/CrashReportWriter.h"
#include "diagnostics/ExceptionTrace.h"
#include "diagnostics/LogRing.h"
#include "diagnostics/LogTee.h"
#include "diagnostics/ThreadName.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <windows.h>

#include <crtdbg.h>
#include <eh.h>

using namespace lakot;

namespace
{
    constexpr std::size_t kWriterStackSize = 1024 * 1024;
    constexpr unsigned long kStackGuaranteeBytes = 64 * 1024;
    constexpr DWORD kWaitMilliseconds = 60000;
    constexpr DWORD kFullDumpWaitMilliseconds = 300000;
    constexpr DWORD kRuntimeFailureExitCode = 3;
    constexpr DWORD kCppExceptionCode = 0xE06D7363;
    constexpr std::size_t kKeptCrashDirectories = 10;

    using PcToFileHeaderFunction = PVOID(NTAPI*)(PVOID, PVOID*);

    CrashReportWriter::Settings sSettings;
    CrashReportWriter::Request sRequest;
    std::atomic<DWORD> sOwner{0};
    std::atomic<DWORD> sWriterThreadId{0};
    HANDLE sRequestEvent = nullptr;
    HANDLE sDoneEvent = nullptr;
    HANDLE sWriterThread = nullptr;
    LogRing* sRing = nullptr;
    PcToFileHeaderFunction sPcToFileHeader = nullptr;

    void copyNarrow(const wchar_t* pWide, char* pOut, std::size_t pCapacity)
    {
        pOut[0] = '\0';

        if (pWide && pWide[0])
        {
            if (WideCharToMultiByte(CP_UTF8, 0, pWide, -1, pOut, static_cast<int>(pCapacity), nullptr, nullptr) <= 0)
            {
                pOut[pCapacity - 1] = '\0';
            }
        }
    }

    void describeFaultAddress(unsigned long long pAddress, char* pOut, std::size_t pCapacity)
    {
        const unsigned long long tExecutableBase = sSettings.executableBase;

        if (pAddress >= tExecutableBase && pAddress < tExecutableBase + sSettings.executableSize)
        {
            std::snprintf(pOut, pCapacity, "%s.exe!0x%llX", sSettings.executableName, pAddress - tExecutableBase);
            return;
        }

        PVOID tBase = nullptr;

        if (sPcToFileHeader && sPcToFileHeader(reinterpret_cast<PVOID>(pAddress), &tBase) && tBase)
        {
            std::snprintf(pOut, pCapacity, "module-0x%llX!0x%llX", reinterpret_cast<unsigned long long>(tBase),
                          pAddress - reinterpret_cast<unsigned long long>(tBase));
            return;
        }

        std::snprintf(pOut, pCapacity, "?!0x%llX", pAddress);
    }

    void buildProvisional(CrashReportWriter::Request& pRequest)
    {
        CrashClassifier::Facts tFacts = {};
        tFacts.source = pRequest.source;
        tFacts.code = pRequest.exceptionRecord.ExceptionCode;
        tFacts.hasCppException = pRequest.hasCppException;
        std::snprintf(tFacts.cppType, sizeof(tFacts.cppType), "%s", pRequest.cppType);
        std::snprintf(tFacts.cppWhat, sizeof(tFacts.cppWhat), "%s", pRequest.cppWhat);
        std::snprintf(tFacts.message, sizeof(tFacts.message), "%s", pRequest.message);

        if (pRequest.source == CrashClassifier::SourceType::eSeh && pRequest.exceptionRecord.NumberParameters >= 2
            && (tFacts.code == 0xC0000005 || tFacts.code == 0xC0000006))
        {
            tFacts.accessType = pRequest.exceptionRecord.ExceptionInformation[0];
            tFacts.address = pRequest.exceptionRecord.ExceptionInformation[1];
            tFacts.hasAddress = true;
        }

        CrashClassifier::Verdict tVerdict;
        CrashClassifier::classify(tFacts, tVerdict);

        char tLocation[128];
        describeFaultAddress(pRequest.context.Rip, tLocation, sizeof(tLocation));

        CrashClassifier::formatLine(pRequest.provisional, sizeof(pRequest.provisional), "CRASH", sSettings.executableName, tVerdict.kind,
                                    tVerdict.description, "?", 0, tLocation, pRequest.threadName, pRequest.threadId);
    }

    [[noreturn]] void reportFatal(CrashClassifier::SourceType pSource, EXCEPTION_POINTERS* pPointers, const char* pMessage)
    {
        const DWORD tThreadId = GetCurrentThreadId();
        DWORD tExpected = 0;

        if (tThreadId == sWriterThreadId.load())
        {
            TerminateProcess(GetCurrentProcess(), kRuntimeFailureExitCode);
        }

        if (!sOwner.compare_exchange_strong(tExpected, tThreadId))
        {
            if (tExpected == tThreadId)
            {
                TerminateProcess(GetCurrentProcess(), kRuntimeFailureExitCode);
            }

            Sleep(INFINITE);
        }

        std::memset(&sRequest, 0, sizeof(sRequest));
        sRequest.source = pSource;
        sRequest.threadId = tThreadId;
        std::snprintf(sRequest.threadName, sizeof(sRequest.threadName), "%s", ThreadName::get());
        sRequest.trace = ExceptionTrace::current();

        DWORD tExitCode = kRuntimeFailureExitCode;

        if (pPointers)
        {
            sRequest.exceptionRecord = *pPointers->ExceptionRecord;
            sRequest.context = *pPointers->ContextRecord;
            tExitCode = pSource == CrashClassifier::SourceType::eSeh ? pPointers->ExceptionRecord->ExceptionCode : kRuntimeFailureExitCode;

            if (sRequest.exceptionRecord.ExceptionCode == kCppExceptionCode && sRequest.exceptionRecord.NumberParameters >= 4)
            {
                sRequest.hasCppException = ExceptionTrace::describe(
                    reinterpret_cast<const void*>(sRequest.exceptionRecord.ExceptionInformation[1]),
                    reinterpret_cast<const void*>(sRequest.exceptionRecord.ExceptionInformation[2]),
                    sRequest.exceptionRecord.ExceptionInformation[3], sRequest.cppType, sizeof(sRequest.cppType), sRequest.cppWhat,
                    sizeof(sRequest.cppWhat));
            }
        }
        else
        {
            RtlCaptureContext(&sRequest.context);
            sRequest.exceptionRecord.ExceptionAddress = reinterpret_cast<PVOID>(sRequest.context.Rip);
        }

        if (pMessage)
        {
            std::snprintf(sRequest.message, sizeof(sRequest.message), "%s", pMessage);
        }

        buildProvisional(sRequest);

        CrashReportWriter tWriter(sSettings);
        const bool tIsWriterUsable = sWriterThread && WaitForSingleObject(sWriterThread, 0) != WAIT_OBJECT_0;

        if (!tIsWriterUsable)
        {
            tWriter.writeProvisional(sRequest);
            tWriter.echoToErrorStream(sRequest.provisional);
        }
        else if (SignalObjectAndWait(sRequestEvent, sDoneEvent, sSettings.isFullDump ? kFullDumpWaitMilliseconds : kWaitMilliseconds, FALSE)
                 != WAIT_OBJECT_0)
        {
            tWriter.echoToErrorStream(sRequest.provisional);
        }

        TerminateProcess(GetCurrentProcess(), tExitCode);
        Sleep(INFINITE);
        std::abort();
    }

    LONG WINAPI onUnhandledException(EXCEPTION_POINTERS* pPointers)
    {
        reportFatal(CrashClassifier::SourceType::eSeh, pPointers, nullptr);
    }

    void onTerminate()
    {
        reportFatal(CrashClassifier::SourceType::eTerminate, nullptr, nullptr);
    }

    void onAbortSignal(int)
    {
        reportFatal(CrashClassifier::SourceType::eAbort, nullptr, nullptr);
    }

    void onPureCall()
    {
        reportFatal(CrashClassifier::SourceType::ePureCall, nullptr, nullptr);
    }

    void onInvalidParameter(const wchar_t* pExpression, const wchar_t* pFunction, const wchar_t*, unsigned, uintptr_t)
    {
        char tExpression[256];
        char tFunction[128];
        char tMessage[420];
        copyNarrow(pExpression, tExpression, sizeof(tExpression));
        copyNarrow(pFunction, tFunction, sizeof(tFunction));
        std::snprintf(tMessage, sizeof(tMessage), "%s in %s", tExpression[0] ? tExpression : "unknown", tFunction[0] ? tFunction : "unknown");
        reportFatal(CrashClassifier::SourceType::eInvalidParameter, nullptr, tMessage);
    }

#if defined(_DEBUG)
    int onCrtReportNarrow(int pType, char* pMessage, int* pReturnValue)
    {
        if (pType == _CRT_WARN)
        {
            *pReturnValue = 0;
            return FALSE;
        }

        char tMessage[512];
        std::snprintf(tMessage, sizeof(tMessage), "%s", pMessage ? pMessage : "");
        reportFatal(CrashClassifier::SourceType::eCrtReport, nullptr, tMessage);
    }

    int onCrtReportWide(int pType, wchar_t* pMessage, int* pReturnValue)
    {
        if (pType == _CRT_WARN)
        {
            *pReturnValue = 0;
            return FALSE;
        }

        char tMessage[512];
        copyNarrow(pMessage, tMessage, sizeof(tMessage));
        reportFatal(CrashClassifier::SourceType::eCrtReport, nullptr, tMessage);
    }
#endif

    DWORD WINAPI writerMain(LPVOID)
    {
        CrashHandler::attachThread("crash-writer");

        WaitForSingleObject(sRequestEvent, INFINITE);

        CrashReportWriter tWriter(sSettings);
        tWriter.write(sRequest);
        SetEvent(sDoneEvent);
        return 0;
    }

    void readEnvironment(const wchar_t* pName, wchar_t* pOut, DWORD pCapacity)
    {
        pOut[0] = L'\0';
        const DWORD tLength = GetEnvironmentVariableW(pName, pOut, pCapacity);

        if (tLength >= pCapacity)
        {
            pOut[0] = L'\0';
        }
    }

    bool resolveBaseDirectory(wchar_t* pOut, DWORD pCapacity)
    {
        readEnvironment(L"LAKOT_CRASH_DIR", pOut, pCapacity);

        if (pOut[0])
        {
            return false;
        }

        wchar_t tTestDirectory[900];
        readEnvironment(L"LAKOT_TEST_LOG_DIR", tTestDirectory, 900);

        if (tTestDirectory[0])
        {
            _snwprintf_s(pOut, pCapacity, _TRUNCATE, L"%s\\crashes", tTestDirectory);
            return false;
        }

        wchar_t tExecutable[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, tExecutable, MAX_PATH);

        wchar_t* tSeparator = std::wcsrchr(tExecutable, L'\\');

        if (tSeparator)
        {
            *tSeparator = L'\0';
        }

        _snwprintf_s(pOut, pCapacity, _TRUNCATE, L"%s\\crashes", tExecutable);
        return true;
    }

    void pruneOldReports()
    {
        namespace fs = std::filesystem;

        std::error_code tError;
        const std::string tPrefix = std::string(sSettings.executableName) + "-";
        std::vector<fs::path> tDirectories;

        for (fs::directory_iterator tIterator(fs::path(sSettings.baseDirectory), tError), tEnd; !tError && tIterator != tEnd;
             tIterator.increment(tError))
        {
            const std::string tName = tIterator->path().filename().string();

            if (tName.rfind(tPrefix, 0) == 0 && fs::exists(tIterator->path() / "summary.txt", tError))
            {
                tDirectories.push_back(tIterator->path());
            }
        }

        if (tDirectories.size() <= kKeptCrashDirectories)
        {
            return;
        }

        std::sort(tDirectories.begin(), tDirectories.end(), std::greater<fs::path>());

        for (std::size_t tIndex = kKeptCrashDirectories; tIndex < tDirectories.size(); ++tIndex)
        {
            fs::remove_all(tDirectories[tIndex], tError);
        }
    }
}

void CrashHandler::install(std::string_view pExecutableName)
{
    std::snprintf(sSettings.executableName, sizeof(sSettings.executableName), "%.*s", static_cast<int>(pExecutableName.size()),
                  pExecutableName.data());

    const bool tIsDefaultBase = resolveBaseDirectory(sSettings.baseDirectory, static_cast<DWORD>(std::size(sSettings.baseDirectory)));

    const HMODULE tExecutable = GetModuleHandleW(nullptr);
    const auto* tDos = reinterpret_cast<const IMAGE_DOS_HEADER*>(tExecutable);
    const auto* tNt = reinterpret_cast<const IMAGE_NT_HEADERS*>(reinterpret_cast<const char*>(tExecutable) + tDos->e_lfanew);
    sSettings.executableBase = reinterpret_cast<unsigned long long>(tExecutable);
    sSettings.executableSize = tNt->OptionalHeader.SizeOfImage;
    sSettings.linkTimestamp = tNt->FileHeader.TimeDateStamp;
    sSettings.errorHandle = GetStdHandle(STD_ERROR_HANDLE);

    wchar_t tFullDump[8] = {};
    readEnvironment(L"LAKOT_CRASH_FULL_DUMP", tFullDump, 8);
    sSettings.isFullDump = tFullDump[0] == L'1';

    HMODULE tNtdll = GetModuleHandleW(L"ntdll.dll");
    sPcToFileHeader = tNtdll ? reinterpret_cast<PcToFileHeaderFunction>(GetProcAddress(tNtdll, "RtlPcToFileHeader")) : nullptr;

    // Leaked on purpose: std::cout may still write during static destruction.
    sRing = new LogRing;
    sSettings.ring = sRing;
    std::cout.rdbuf(new LogTee(*std::cout.rdbuf(), *sRing));
    std::cerr.rdbuf(new LogTee(*std::cerr.rdbuf(), *sRing));
    std::clog.rdbuf(new LogTee(*std::clog.rdbuf(), *sRing));

    ExceptionTrace::install();

    sRequestEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    sDoneEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    DWORD tWriterId = 0;
    sWriterThread = CreateThread(nullptr, kWriterStackSize, writerMain, nullptr, STACK_SIZE_PARAM_IS_A_RESERVATION, &tWriterId);
    sWriterThreadId.store(tWriterId);

    SetUnhandledExceptionFilter(onUnhandledException);
    _set_invalid_parameter_handler(onInvalidParameter);
    _set_purecall_handler(onPureCall);
    std::signal(SIGABRT, onAbortSignal);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    SetErrorMode(GetErrorMode() | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_error_mode(_OUT_TO_STDERR);

#if defined(_DEBUG)
    _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, onCrtReportNarrow);
    _CrtSetReportHookW2(_CRT_RPTHOOK_INSTALL, onCrtReportWide);
#endif

    attachThread("main");

    if (tIsDefaultBase)
    {
        pruneOldReports();
    }
}

void CrashHandler::attachThread(std::string_view pThreadName)
{
    ThreadName::set(pThreadName);

    ULONG tGuarantee = kStackGuaranteeBytes;
    SetThreadStackGuarantee(&tGuarantee);

    std::set_terminate(onTerminate);
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
    return sSettings.executableName[0] ? sSettings.executableName : "unknown";
}
