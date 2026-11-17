#include "diagnostics/CrashReportWriter.h"

#include "LakotBuildInfo.h"
#include "diagnostics/LogRing.h"
#include "diagnostics/StackSymbolizer.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <fstream>

#include <dbghelp.h>
#include <psapi.h>

using namespace lakot;

namespace
{
    constexpr std::size_t kMaxFrames = 8192;
    constexpr std::size_t kTopFrameScanLimit = 1024;
    constexpr std::size_t kInFlightScanLimit = 64;
    constexpr std::size_t kReportLogLines = 200;
    constexpr std::size_t kMaxPrintedStackLines = 220;
    constexpr unsigned kSymbolizerLockWaitMilliseconds = 2000;
    constexpr unsigned kLogLockWaitMilliseconds = 200;
    constexpr unsigned kCollapseThreshold = 3;

    StackSymbolizer::RawFrame sFrames[kMaxFrames];

    void append(std::string& pOut, const char* pFormat, ...)
    {
        char tBuffer[2048];
        va_list tArguments;
        va_start(tArguments, pFormat);
        const int tWritten = std::vsnprintf(tBuffer, sizeof(tBuffer), pFormat, tArguments);
        va_end(tArguments);

        if (tWritten > 0)
        {
            pOut.append(tBuffer, tWritten < static_cast<int>(sizeof(tBuffer)) ? static_cast<std::size_t>(tWritten) : sizeof(tBuffer) - 1);
        }
    }

    bool writeFile(const wchar_t* pPath, const char* pData, std::size_t pSize)
    {
        HANDLE tFile = CreateFileW(pPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

        if (tFile == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        DWORD tWritten = 0;
        const BOOL tResult = WriteFile(tFile, pData, static_cast<DWORD>(pSize), &tWritten, nullptr);
        CloseHandle(tFile);
        return tResult != FALSE;
    }

    bool createTree(const wchar_t* pPath)
    {
        wchar_t tBuffer[1200];
        wcsncpy_s(tBuffer, pPath, _TRUNCATE);

        for (wchar_t* tCursor = tBuffer + 3; *tCursor; ++tCursor)
        {
            if (*tCursor == L'\\' || *tCursor == L'/')
            {
                const wchar_t tSaved = *tCursor;
                *tCursor = L'\0';
                CreateDirectoryW(tBuffer, nullptr);
                *tCursor = tSaved;
            }
        }

        CreateDirectoryW(tBuffer, nullptr);
        const DWORD tAttributes = GetFileAttributesW(tBuffer);
        return tAttributes != INVALID_FILE_ATTRIBUTES && (tAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    std::string narrow(const wchar_t* pWide)
    {
        if (!pWide || !pWide[0])
        {
            return {};
        }

        const int tSize = WideCharToMultiByte(CP_UTF8, 0, pWide, -1, nullptr, 0, nullptr, nullptr);

        if (tSize <= 1)
        {
            return {};
        }

        std::string tText(static_cast<std::size_t>(tSize), '\0');
        WideCharToMultiByte(CP_UTF8, 0, pWide, -1, tText.data(), tSize, nullptr, nullptr);
        tText.resize(static_cast<std::size_t>(tSize) - 1);
        return tText;
    }

    std::wstring widen(const std::string& pText)
    {
        if (pText.empty())
        {
            return {};
        }

        const int tSize = MultiByteToWideChar(CP_UTF8, 0, pText.c_str(), -1, nullptr, 0);

        if (tSize <= 1)
        {
            return {};
        }

        std::wstring tWide(static_cast<std::size_t>(tSize), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, pText.c_str(), -1, tWide.data(), tSize);
        tWide.resize(static_cast<std::size_t>(tSize) - 1);
        return tWide;
    }

    bool equalsIgnoreCase(const char* pLeft, const char* pRight, std::size_t pLength)
    {
        return _strnicmp(pLeft, pRight, pLength) == 0;
    }

    bool containsIgnoreCase(const std::string& pText, const char* pNeedle)
    {
        const std::size_t tLength = std::strlen(pNeedle);

        for (std::size_t tIndex = 0; tIndex + tLength <= pText.size(); ++tIndex)
        {
            if (equalsIgnoreCase(pText.c_str() + tIndex, pNeedle, tLength))
            {
                return true;
            }
        }

        return false;
    }

    void describeFrameLocation(const StackSymbolizer::Frame& pFrame, char* pFile, std::size_t pFileCapacity, unsigned& pLine,
                               char* pFunction, std::size_t pFunctionCapacity)
    {
        if (pFrame.hasLine)
        {
            CrashClassifier::toSourceRelative(pFrame.file, pFile, pFileCapacity);
            pLine = pFrame.line;
        }
        else
        {
            std::snprintf(pFile, pFileCapacity, "?");
            pLine = 0;
        }

        if (pFrame.hasSymbol && pFrame.function[0])
        {
            std::snprintf(pFunction, pFunctionCapacity, "%s", pFrame.function);
        }
        else
        {
            std::snprintf(pFunction, pFunctionCapacity, "%s!0x%llX", pFrame.module[0] ? pFrame.module : "?",
                          pFrame.address - pFrame.moduleBase);
        }
    }

    bool frameNamed(const StackSymbolizer::Frame& pFrame, const char* pName)
    {
        const char* tFunction = pFrame.function[0] == '_' ? pFrame.function + 1 : pFrame.function;
        return std::strcmp(tFunction, pName) == 0;
    }

    std::string formatTime(const SYSTEMTIME& pTime)
    {
        char tBuffer[64];
        std::snprintf(tBuffer, sizeof(tBuffer), "%04u-%02u-%02u %02u:%02u:%02u", pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour,
                      pTime.wMinute, pTime.wSecond);
        return tBuffer;
    }

    std::string describeOperatingSystem()
    {
        using RtlGetVersionFunction = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
        HMODULE tNtdll = GetModuleHandleW(L"ntdll.dll");
        auto tFunction = tNtdll ? reinterpret_cast<RtlGetVersionFunction>(GetProcAddress(tNtdll, "RtlGetVersion")) : nullptr;

        RTL_OSVERSIONINFOW tInfo = {};
        tInfo.dwOSVersionInfoSize = sizeof(tInfo);

        if (!tFunction || tFunction(&tInfo) != 0)
        {
            return "unknown";
        }

        char tBuffer[64];
        std::snprintf(tBuffer, sizeof(tBuffer), "Windows %lu.%lu build %lu", tInfo.dwMajorVersion, tInfo.dwMinorVersion, tInfo.dwBuildNumber);
        return tBuffer;
    }

    void appendProcessSection(std::string& pOut, const CrashReportWriter::Settings& pSettings, const SYSTEMTIME& pLocal, const SYSTEMTIME& pUtc)
    {
        wchar_t tPath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, tPath, MAX_PATH);

        append(pOut, "== process ==\n");
        append(pOut, "executable: %s\n", narrow(tPath).c_str());
        append(pOut, "pid: %lu\n", GetCurrentProcessId());
        append(pOut, "command line: %s\n", narrow(GetCommandLineW()).c_str());
        append(pOut, "crash time: %s local, %s UTC\n", formatTime(pLocal).c_str(), formatTime(pUtc).c_str());

        FILETIME tCreation, tExit, tKernel, tUser;

        if (GetProcessTimes(GetCurrentProcess(), &tCreation, &tExit, &tKernel, &tUser))
        {
            FILETIME tNow;
            GetSystemTimeAsFileTime(&tNow);
            ULARGE_INTEGER tStart, tEnd;
            tStart.LowPart = tCreation.dwLowDateTime;
            tStart.HighPart = tCreation.dwHighDateTime;
            tEnd.LowPart = tNow.dwLowDateTime;
            tEnd.HighPart = tNow.dwHighDateTime;
            append(pOut, "uptime: %.1f s\n", static_cast<double>(tEnd.QuadPart - tStart.QuadPart) / 10000000.0);
        }

        SYSTEMTIME tLinked = {};
        FILETIME tLinkedFile;
        ULARGE_INTEGER tLinkedValue;
        tLinkedValue.QuadPart = 116444736000000000ull + static_cast<unsigned long long>(pSettings.linkTimestamp) * 10000000ull;
        tLinkedFile.dwLowDateTime = tLinkedValue.LowPart;
        tLinkedFile.dwHighDateTime = tLinkedValue.HighPart;
        FileTimeToSystemTime(&tLinkedFile, &tLinked);

#if defined(NDEBUG)
        const char* tBuildType = "Release";
#else
        const char* tBuildType = "Debug";
#endif

        append(pOut, "build: %s %s linked %s UTC\n", LAKOT_BUILD_COMMIT, tBuildType, formatTime(tLinked).c_str());
        append(pOut, "os: %s\n", describeOperatingSystem().c_str());

        SYSTEM_INFO tSystem;
        GetNativeSystemInfo(&tSystem);
        append(pOut, "cpus: %lu\n", tSystem.dwNumberOfProcessors);

        MEMORYSTATUSEX tMemory = {};
        tMemory.dwLength = sizeof(tMemory);

        if (GlobalMemoryStatusEx(&tMemory))
        {
            append(pOut, "memory load: %lu%%, physical free %llu MB\n", tMemory.dwMemoryLoad, tMemory.ullAvailPhys / (1024 * 1024));
        }

        PROCESS_MEMORY_COUNTERS tCounters = {};

        if (K32GetProcessMemoryInfo(GetCurrentProcess(), &tCounters, sizeof(tCounters)))
        {
            append(pOut, "working set: %llu MB, peak %llu MB\n", static_cast<unsigned long long>(tCounters.WorkingSetSize) / (1024 * 1024),
                   static_cast<unsigned long long>(tCounters.PeakWorkingSetSize) / (1024 * 1024));
        }

        append(pOut, "\n");
    }

    void appendEnvironmentSection(std::string& pOut)
    {
        append(pOut, "== environment ==\n");

        wchar_t tDirectory[MAX_PATH] = {};
        GetCurrentDirectoryW(MAX_PATH, tDirectory);
        append(pOut, "cwd: %s\n", narrow(tDirectory).c_str());

        LPWCH tBlock = GetEnvironmentStringsW();

        for (const wchar_t* tEntry = tBlock; tBlock && *tEntry; tEntry += std::wcslen(tEntry) + 1)
        {
            const std::string tText = narrow(tEntry);
            const std::size_t tEquals = tText.find('=');
            const std::string tName = tText.substr(0, tEquals);

            if (tText.size() < 6 || !equalsIgnoreCase(tText.c_str(), "LAKOT_", 6) || containsIgnoreCase(tName, "PASSWORD")
                || containsIgnoreCase(tName, "SECRET") || containsIgnoreCase(tName, "TOKEN") || containsIgnoreCase(tName, "KEY"))
            {
                continue;
            }

            append(pOut, "%s\n", tText.c_str());
        }

        if (tBlock)
        {
            FreeEnvironmentStringsW(tBlock);
        }

        append(pOut, "\n");
    }

    void appendRegisterSection(std::string& pOut, const CONTEXT& pContext)
    {
        append(pOut, "== registers ==\n");
        append(pOut, "RIP=%016llX RSP=%016llX RBP=%016llX EFLAGS=%08lX\n", pContext.Rip, pContext.Rsp, pContext.Rbp, pContext.EFlags);
        append(pOut, "RAX=%016llX RBX=%016llX RCX=%016llX RDX=%016llX\n", pContext.Rax, pContext.Rbx, pContext.Rcx, pContext.Rdx);
        append(pOut, "RSI=%016llX RDI=%016llX R8 =%016llX R9 =%016llX\n", pContext.Rsi, pContext.Rdi, pContext.R8, pContext.R9);
        append(pOut, "R10=%016llX R11=%016llX R12=%016llX R13=%016llX\n", pContext.R10, pContext.R11, pContext.R12, pContext.R13);
        append(pOut, "R14=%016llX R15=%016llX\n\n", pContext.R14, pContext.R15);
    }

    void appendStackSection(std::string& pOut, StackSymbolizer& pSymbolizer, std::size_t pCount, const CrashReportWriter::Request& pRequest)
    {
        append(pOut, "== stack (thread %s:%lu) ==\n", pRequest.threadName, pRequest.threadId);

        std::size_t tPrinted = 0;

        for (std::size_t tIndex = 0; tIndex < pCount && tPrinted < kMaxPrintedStackLines;)
        {
            std::size_t tRun = 1;

            while (tIndex + tRun < pCount && sFrames[tIndex + tRun].address == sFrames[tIndex].address
                   && sFrames[tIndex + tRun].inlineContext == sFrames[tIndex].inlineContext)
            {
                ++tRun;
            }

            const std::size_t tShown = tRun >= kCollapseThreshold ? 1 : tRun;

            for (std::size_t tCopy = 0; tCopy < tShown; ++tCopy)
            {
                StackSymbolizer::Frame tFrame;
                pSymbolizer.resolve(sFrames[tIndex], tFrame);

                const bool tIsOurs = CrashClassifier::isOurCode(tFrame.isMainModule && tFrame.hasLine, tFrame.file);
                char tFile[260];
                char tFunction[300];
                unsigned tLine = 0;
                describeFrameLocation(tFrame, tFile, sizeof(tFile), tLine, tFunction, sizeof(tFunction));

                append(pOut, "#%02zu%c 0x%016llX %s!%s+0x%llX", tIndex + tCopy, tIsOurs ? '*' : ' ', tFrame.address,
                       tFrame.module[0] ? tFrame.module : "?", tFrame.hasSymbol ? tFrame.function : "?", tFrame.displacement);

                if (tFrame.hasLine)
                {
                    append(pOut, " [%s:%u]", tFile, tLine);
                }

                append(pOut, "\n");
                ++tPrinted;
            }

            if (tRun >= kCollapseThreshold)
            {
                append(pOut, "    ... repeated %zu times\n", tRun);
                ++tPrinted;
            }

            tIndex += tRun;
        }

        if (tPrinted >= kMaxPrintedStackLines)
        {
            append(pOut, "    ... stack output truncated\n");
        }

        append(pOut, "\n");
    }

    void appendModuleSection(std::string& pOut, StackSymbolizer& pSymbolizer)
    {
        append(pOut, "== modules ==\n");

        for (const StackSymbolizer::ModuleInfo& tModule : pSymbolizer.listModules())
        {
            append(pOut, "0x%016llX %8X %-10s %s\n", tModule.base, tModule.size, tModule.symbolState, narrow(tModule.path.c_str()).c_str());
        }

        append(pOut, "\n");
    }

    void appendSourceSnippet(std::string& pOut, const StackSymbolizer::Frame& pTop, const char* pRelativeFile, unsigned pLine)
    {
        if (!pTop.hasLine || pLine == 0)
        {
            return;
        }

        std::ifstream tStream(widen(pTop.file));

        if (!tStream.is_open())
        {
            tStream.open(widen(std::string(LAKOT_SOURCE_ROOT) + "/" + pRelativeFile));
        }

        if (!tStream.is_open())
        {
            return;
        }

        append(pOut, "source: %s:%u\n", pRelativeFile, pLine);

        std::string tText;
        unsigned tNumber = 0;

        while (std::getline(tStream, tText) && tNumber < pLine + 3)
        {
            ++tNumber;

            if (tNumber + 3 >= pLine)
            {
                while (!tText.empty() && (tText.back() == '\r' || tText.back() == '\n'))
                {
                    tText.pop_back();
                }

                append(pOut, "  %s %5u | %s\n", tNumber == pLine ? ">" : " ", tNumber, tText.c_str());
            }
        }
    }
}

CrashReportWriter::CrashReportWriter(const Settings& pSettings)
    : mSettings(pSettings)
    , mDirectory{}
    , mHasDirectory(false)
{
}

void CrashReportWriter::buildPath(wchar_t* pOut, std::size_t pCapacity, const wchar_t* pFileName) const
{
    _snwprintf_s(pOut, pCapacity, _TRUNCATE, L"%s\\%s", mDirectory, pFileName);
}

void CrashReportWriter::ensureDirectory()
{
    if (mHasDirectory)
    {
        return;
    }

    SYSTEMTIME tNow;
    GetLocalTime(&tNow);

    wchar_t tName[160];
    _snwprintf_s(tName, _TRUNCATE, L"%S-%04u%02u%02u-%02u%02u%02u-%lu", mSettings.executableName, tNow.wYear, tNow.wMonth, tNow.wDay,
                 tNow.wHour, tNow.wMinute, tNow.wSecond, GetCurrentProcessId());

    wchar_t tBase[1024];
    wcsncpy_s(tBase, mSettings.baseDirectory, _TRUNCATE);

    for (int tAttempt = 0; tAttempt < 2 && !mHasDirectory; ++tAttempt)
    {
        _snwprintf_s(mDirectory, _TRUNCATE, L"%s\\%s", tBase, tName);
        mHasDirectory = createTree(mDirectory);

        if (!mHasDirectory)
        {
            wchar_t tLocal[MAX_PATH] = {};

            if (GetEnvironmentVariableW(L"LOCALAPPDATA", tLocal, MAX_PATH) == 0)
            {
                break;
            }

            _snwprintf_s(tBase, _TRUNCATE, L"%s\\Lakot\\crashes", tLocal);
        }
    }
}

void CrashReportWriter::echoToErrorStream(const char* pLine) const
{
    if (!mSettings.errorHandle || mSettings.errorHandle == INVALID_HANDLE_VALUE)
    {
        return;
    }

    DWORD tWritten = 0;
    WriteFile(mSettings.errorHandle, pLine, static_cast<DWORD>(std::strlen(pLine)), &tWritten, nullptr);
    WriteFile(mSettings.errorHandle, "\r\n", 2, &tWritten, nullptr);
}

void CrashReportWriter::writeProvisional(const Request& pRequest)
{
    ensureDirectory();

    if (!mHasDirectory)
    {
        return;
    }

    char tContent[1200];
    const int tLength = std::snprintf(tContent, sizeof(tContent), "%s\nnote: provisional, symbolization did not finish\n", pRequest.provisional);

    wchar_t tPath[1300];
    buildPath(tPath, std::size(tPath), L"summary.txt");

    if (tLength > 0)
    {
        writeFile(tPath, tContent, static_cast<std::size_t>(tLength) < sizeof(tContent) ? static_cast<std::size_t>(tLength) : sizeof(tContent) - 1);
    }
}

void CrashReportWriter::writeDump(Request& pRequest)
{
    wchar_t tPath[1300];
    buildPath(tPath, std::size(tPath), L"minidump.dmp");

    HANDLE tFile = CreateFileW(tPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (tFile == INVALID_HANDLE_VALUE)
    {
        return;
    }

    EXCEPTION_POINTERS tPointers = {&pRequest.exceptionRecord, &pRequest.context};

    MINIDUMP_EXCEPTION_INFORMATION tException = {};
    tException.ThreadId = pRequest.threadId;
    tException.ExceptionPointers = &tPointers;
    tException.ClientPointers = FALSE;

    unsigned tType = MiniDumpWithThreadInfo | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithUnloadedModules | MiniDumpWithHandleData;

    if (mSettings.isFullDump)
    {
        tType |= MiniDumpWithFullMemory;
    }

    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), tFile, static_cast<MINIDUMP_TYPE>(tType), &tException, nullptr, nullptr);
    CloseHandle(tFile);
}

void CrashReportWriter::write(Request& pRequest)
{
    ensureDirectory();
    writeProvisional(pRequest);

    StackSymbolizer tSymbolizer(kSymbolizerLockWaitMilliseconds);

    if (mHasDirectory)
    {
        writeDump(pRequest);
    }

    writeFinalReports(pRequest, tSymbolizer);
}

void CrashReportWriter::writeFinalReports(Request& pRequest, StackSymbolizer& pSymbolizer)
{
    std::size_t tCount = 0;
    HANDLE tThread = OpenThread(THREAD_ALL_ACCESS, FALSE, pRequest.threadId);

    if (tThread)
    {
        tCount = pSymbolizer.walk(pRequest.context, tThread, sFrames, kMaxFrames);
        CloseHandle(tThread);
    }

    if (tCount == 0)
    {
        sFrames[0] = {pRequest.context.Rip, INLINE_FRAME_CONTEXT_IGNORE, false};
        tCount = 1;
    }

    StackSymbolizer::Frame tTop;
    std::size_t tTopIndex = 0;
    pSymbolizer.findTopFrame(sFrames, tCount, kTopFrameScanLimit, tTopIndex, tTop);

    CrashClassifier::Facts tFacts = {};
    tFacts.source = pRequest.source;
    tFacts.code = pRequest.exceptionRecord.ExceptionCode;
    tFacts.hasCppException = pRequest.hasCppException;
    std::snprintf(tFacts.cppType, sizeof(tFacts.cppType), "%s", pRequest.cppType);
    std::snprintf(tFacts.cppWhat, sizeof(tFacts.cppWhat), "%s", pRequest.cppWhat);
    std::snprintf(tFacts.message, sizeof(tFacts.message), "%s", pRequest.message);
    std::snprintf(tFacts.topFunction, sizeof(tFacts.topFunction), "%s", tTop.function);

    if (pRequest.source == CrashClassifier::SourceType::eSeh && pRequest.exceptionRecord.NumberParameters >= 2
        && (tFacts.code == 0xC0000005 || tFacts.code == 0xC0000006))
    {
        tFacts.accessType = pRequest.exceptionRecord.ExceptionInformation[0];
        tFacts.address = pRequest.exceptionRecord.ExceptionInformation[1];
        tFacts.hasAddress = true;
    }

    bool tHasThrowFrame = false;
    const std::size_t tScan = tCount < kInFlightScanLimit ? tCount : kInFlightScanLimit;

    for (std::size_t tIndex = 0; tIndex < tScan; ++tIndex)
    {
        StackSymbolizer::Frame tFrame;
        pSymbolizer.resolve(sFrames[tIndex], tFrame);

        if (frameNamed(tFrame, "CxxThrowException"))
        {
            tHasThrowFrame = true;
        }
        else if (frameNamed(tFrame, "wassert") || frameNamed(tFrame, "assert"))
        {
            tFacts.hasAssertFrame = true;
        }
    }

    const ExceptionTrace::Record& tTrace = pRequest.trace;
    const bool tCanDescribeTrace = tHasThrowFrame && tTrace.sequence != 0;
    const bool tIsTerminateLike = pRequest.source == CrashClassifier::SourceType::eTerminate
                                  || (pRequest.source == CrashClassifier::SourceType::eAbort && !tFacts.hasAssertFrame);

    if (tIsTerminateLike && tCanDescribeTrace)
    {
        tFacts.source = CrashClassifier::SourceType::eTerminate;
        tFacts.hasCppException = ExceptionTrace::describe(tTrace.object, tTrace.throwInfo, tTrace.imageBase, tFacts.cppType,
                                                          sizeof(tFacts.cppType), tFacts.cppWhat, sizeof(tFacts.cppWhat));
        tFacts.isRethrow = tTrace.isRethrow;
    }
    else if (pRequest.source == CrashClassifier::SourceType::eTerminate)
    {
        tFacts.hasCppException = false;
    }
    else if (tHasThrowFrame && tTrace.sequence != 0 && pRequest.source != CrashClassifier::SourceType::eAbort)
    {
        char tIgnored[8];
        tFacts.isUnwinding = ExceptionTrace::describe(tTrace.object, tTrace.throwInfo, tTrace.imageBase, tFacts.unwindType,
                                                      sizeof(tFacts.unwindType), tIgnored, sizeof(tIgnored));
    }

    if (tCount > 2)
    {
        for (std::size_t tIndex = 1; tIndex < tCount; ++tIndex)
        {
            if (sFrames[tIndex].address == sFrames[1].address)
            {
                ++tFacts.repeatCount;
            }
        }
    }

    CrashClassifier::Verdict tVerdict;
    CrashClassifier::classify(tFacts, tVerdict);

    char tFile[260];
    char tFunction[300];
    unsigned tLine = 0;
    describeFrameLocation(tTop, tFile, sizeof(tFile), tLine, tFunction, sizeof(tFunction));

    char tLineOne[1024];
    CrashClassifier::formatLine(tLineOne, sizeof(tLineOne), "CRASH", mSettings.executableName, tVerdict.kind, tVerdict.description, tFile,
                                tLine, tFunction, pRequest.threadName, pRequest.threadId);

    wchar_t tReportPath[1300];
    wchar_t tDumpPath[1300];
    buildPath(tReportPath, std::size(tReportPath), L"report.txt");
    buildPath(tDumpPath, std::size(tDumpPath), L"minidump.dmp");

    SYSTEMTIME tLocal;
    SYSTEMTIME tUtc;
    GetLocalTime(&tLocal);
    GetSystemTime(&tUtc);

    std::string tSummary;
    append(tSummary, "%s\n", tLineOne);

    for (unsigned tCause = 0; tCause < tVerdict.causeCount; ++tCause)
    {
        append(tSummary, "cause: %s\n", tVerdict.cause[tCause]);
    }

    unsigned tStackLines = 0;
    std::string tPreviousFunction = tTop.function;

    for (std::size_t tIndex = tTopIndex + 1; tIndex < tCount && tIndex < kTopFrameScanLimit && tStackLines < 5; ++tIndex)
    {
        StackSymbolizer::Frame tFrame;
        pSymbolizer.resolve(sFrames[tIndex], tFrame);

        if (!CrashClassifier::isOurCode(tFrame.isMainModule && tFrame.hasLine, tFrame.file) || tPreviousFunction == tFrame.function)
        {
            continue;
        }

        char tFrameFile[260];
        char tFrameFunction[300];
        unsigned tFrameLine = 0;
        describeFrameLocation(tFrame, tFrameFile, sizeof(tFrameFile), tFrameLine, tFrameFunction, sizeof(tFrameFunction));
        append(tSummary, "stack: %s:%u in %s\n", tFrameFile, tFrameLine, tFrameFunction);
        tPreviousFunction = tFrame.function;
        ++tStackLines;
    }

    appendSourceSnippet(tSummary, tTop, tFile, tLine);

#if defined(NDEBUG)
    const char* tBuildType = "Release";
#else
    const char* tBuildType = "Debug";
#endif

    FILETIME tLinkedFile;
    ULARGE_INTEGER tLinkedValue;
    tLinkedValue.QuadPart = 116444736000000000ull + static_cast<unsigned long long>(mSettings.linkTimestamp) * 10000000ull;
    tLinkedFile.dwLowDateTime = tLinkedValue.LowPart;
    tLinkedFile.dwHighDateTime = tLinkedValue.HighPart;
    SYSTEMTIME tLinked = {};
    FileTimeToSystemTime(&tLinkedFile, &tLinked);

    append(tSummary, "build: %s %s linked %s UTC\n", LAKOT_BUILD_COMMIT, tBuildType, formatTime(tLinked).c_str());
    append(tSummary, "report: %s\n", narrow(tReportPath).c_str());
    append(tSummary, "dump: %s\n", narrow(tDumpPath).c_str());

    std::string tReport;
    append(tReport, "== crash ==\n%s", tSummary.c_str());

    StackSymbolizer::Frame tFault;
    pSymbolizer.resolve({pRequest.context.Rip, INLINE_FRAME_CONTEXT_IGNORE, false}, tFault);
    append(tReport, "exception code: 0x%08lX flags 0x%lX at %s+0x%llX\n", pRequest.exceptionRecord.ExceptionCode,
           pRequest.exceptionRecord.ExceptionFlags, tFault.module[0] ? tFault.module : "?", tFault.address - tFault.moduleBase);

    if (tFacts.hasAddress)
    {
        append(tReport, "access: %s 0x%016llX\n", tFacts.accessType == 0 ? "read" : tFacts.accessType == 1 ? "write" : "execute", tFacts.address);
    }

    if (tFacts.hasCppException)
    {
        append(tReport, "c++ exception: %s what=\"%s\"\n", tFacts.cppType, tFacts.cppWhat);
    }

    if (pRequest.message[0])
    {
        append(tReport, "message: %s\n", pRequest.message);
    }

    append(tReport, "\n");
    appendProcessSection(tReport, mSettings, tLocal, tUtc);
    appendEnvironmentSection(tReport);
    appendRegisterSection(tReport, pRequest.context);
    appendStackSection(tReport, pSymbolizer, tCount, pRequest);
    appendModuleSection(tReport, pSymbolizer);

    append(tReport, "== log (last %zu lines) ==\n", kReportLogLines);

    if (mSettings.ring)
    {
        std::string tLog(LogRing::kCapacity + 1, '\0');
        const std::size_t tLogLength = mSettings.ring->copyTail(tLog.data(), tLog.size(), kReportLogLines, kLogLockWaitMilliseconds);
        tReport.append(tLog.data(), tLogLength);
    }

    wchar_t tSummaryPath[1300];
    buildPath(tSummaryPath, std::size(tSummaryPath), L"summary.txt");

    if (mHasDirectory)
    {
        writeFile(tSummaryPath, tSummary.data(), tSummary.size());
        writeFile(tReportPath, tReport.data(), tReport.size());
    }

    echoToErrorStream(tLineOne);
}
