#include "diagnostics/StackSymbolizer.h"

#include "diagnostics/CrashClassifier.h"

#include <cstring>
#include <cwchar>

#include <dbghelp.h>

using namespace lakot;

namespace
{
    SRWLOCK sLock = SRWLOCK_INIT;
    bool sIsInitialized = false;

    struct SymbolBuffer
    {
        SYMBOL_INFOW info;
        WCHAR name[255];
    };

    void toNarrow(const wchar_t* pWide, char* pOut, std::size_t pCapacity)
    {
        pOut[0] = '\0';

        if (pWide && pWide[0])
        {
            const int tWritten = WideCharToMultiByte(CP_UTF8, 0, pWide, -1, pOut, static_cast<int>(pCapacity), nullptr, nullptr);

            if (tWritten <= 0)
            {
                pOut[pCapacity - 1] = '\0';
            }
        }
    }

    bool isPlainContext(unsigned pContext)
    {
        return pContext == INLINE_FRAME_CONTEXT_IGNORE || pContext == INLINE_FRAME_CONTEXT_INIT;
    }

    const char* describeSymbolType(SYM_TYPE pType)
    {
        switch (pType)
        {
        case SymPdb:
            return "pdb";
        case SymExport:
            return "export";
        case SymNone:
            return "none";
        case SymDeferred:
            return "not-loaded";
        default:
            return "other";
        }
    }

    BOOL CALLBACK onModule(PCWSTR pName, DWORD64 pBase, ULONG pSize, PVOID pContext)
    {
        auto* tModules = static_cast<std::vector<StackSymbolizer::ModuleInfo>*>(pContext);

        IMAGEHLP_MODULEW64 tInfo = {};
        tInfo.SizeOfStruct = sizeof(tInfo);
        const bool tHasInfo = SymGetModuleInfoW64(GetCurrentProcess(), pBase, &tInfo) != FALSE;

        StackSymbolizer::ModuleInfo tModule;
        tModule.path = pName ? pName : L"";
        tModule.base = pBase;
        tModule.size = pSize;
        tModule.symbolState = tHasInfo ? describeSymbolType(tInfo.SymType) : "none";
        tModules->push_back(std::move(tModule));

        return TRUE;
    }
}

StackSymbolizer::StackSymbolizer(unsigned pLockWaitMilliseconds)
    : mOwnsLock(false)
{
    if (pLockWaitMilliseconds == kWaitForever)
    {
        AcquireSRWLockExclusive(&sLock);
        mOwnsLock = true;
    }
    else
    {
        mOwnsLock = TryAcquireSRWLockExclusive(&sLock) != FALSE;

        for (unsigned tWaited = 0; !mOwnsLock && tWaited < pLockWaitMilliseconds; ++tWaited)
        {
            Sleep(1);
            mOwnsLock = TryAcquireSRWLockExclusive(&sLock) != FALSE;
        }
    }

    initialize();
}

StackSymbolizer::~StackSymbolizer()
{
    if (mOwnsLock)
    {
        ReleaseSRWLockExclusive(&sLock);
    }
}

bool StackSymbolizer::isLockOwned() const
{
    return mOwnsLock;
}

void StackSymbolizer::initialize()
{
    const HANDLE tProcess = GetCurrentProcess();

    if (sIsInitialized)
    {
        SymRefreshModuleList(tProcess);
        return;
    }

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_NO_PROMPTS);

    wchar_t tDirectory[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, tDirectory, MAX_PATH);

    wchar_t* tSeparator = std::wcsrchr(tDirectory, L'\\');

    if (tSeparator)
    {
        *tSeparator = L'\0';
    }

    sIsInitialized = SymInitializeW(tProcess, tDirectory, TRUE) != FALSE;
}

std::size_t StackSymbolizer::walk(const CONTEXT& pContext, HANDLE pThread, RawFrame* pOut, std::size_t pCapacity)
{
    CONTEXT tContext = pContext;
    const HANDLE tProcess = GetCurrentProcess();

    STACKFRAME_EX tFrame = {};
    tFrame.StackFrameSize = sizeof(tFrame);
    tFrame.AddrPC.Offset = tContext.Rip;
    tFrame.AddrPC.Mode = AddrModeFlat;
    tFrame.AddrFrame.Offset = tContext.Rbp;
    tFrame.AddrFrame.Mode = AddrModeFlat;
    tFrame.AddrStack.Offset = tContext.Rsp;
    tFrame.AddrStack.Mode = AddrModeFlat;
    tFrame.InlineFrameContext = INLINE_FRAME_CONTEXT_INIT;

    std::size_t tCount = 0;

    while (tCount < pCapacity)
    {
        if (!StackWalkEx(IMAGE_FILE_MACHINE_AMD64, tProcess, pThread, &tFrame, &tContext, nullptr, SymFunctionTableAccess64,
                         SymGetModuleBase64, nullptr, SYM_STKWALK_DEFAULT))
        {
            break;
        }

        if (tFrame.AddrPC.Offset == 0)
        {
            break;
        }

        const bool tIsFirst = tCount == 0;
        const bool tIsInlineOfFirst = !tIsFirst && tFrame.AddrPC.Offset == pOut[0].address;

        pOut[tCount].address = tFrame.AddrPC.Offset;
        pOut[tCount].inlineContext = tFrame.InlineFrameContext;
        pOut[tCount].isReturnAddress = !tIsFirst && !tIsInlineOfFirst;
        ++tCount;
    }

    return tCount;
}

std::size_t StackSymbolizer::expandReturnAddresses(void* const* pAddresses, std::size_t pCount, RawFrame* pOut, std::size_t pCapacity)
{
    const HANDLE tProcess = GetCurrentProcess();
    std::size_t tCount = 0;

    for (std::size_t tIndex = 0; tIndex < pCount && tCount < pCapacity; ++tIndex)
    {
        const unsigned long long tAddress = reinterpret_cast<unsigned long long>(pAddresses[tIndex]) - 1;
        const DWORD tInlineCount = SymAddrIncludeInlineTrace(tProcess, tAddress);

        if (tInlineCount > 0)
        {
            DWORD tContext = 0;
            DWORD tFrameIndex = 0;

            if (SymQueryInlineTrace(tProcess, tAddress, INLINE_FRAME_CONTEXT_INIT, tAddress, tAddress, &tContext, &tFrameIndex))
            {
                for (DWORD tInline = 0; tInline < tInlineCount && tCount < pCapacity; ++tInline)
                {
                    pOut[tCount++] = {tAddress + 1, tContext + tInline, true};
                }
            }
        }

        if (tCount < pCapacity)
        {
            pOut[tCount++] = {tAddress + 1, INLINE_FRAME_CONTEXT_IGNORE, true};
        }
    }

    return tCount;
}

bool StackSymbolizer::resolve(const RawFrame& pRaw, Frame& pFrame)
{
    std::memset(&pFrame, 0, sizeof(pFrame));

    const HANDLE tProcess = GetCurrentProcess();
    const unsigned long long tAddress = pRaw.address;
    const unsigned long long tLookup = pRaw.isReturnAddress && tAddress > 0 ? tAddress - 1 : tAddress;
    const unsigned long long tAdjustment = tAddress - tLookup;

    pFrame.address = tAddress;
    pFrame.moduleBase = SymGetModuleBase64(tProcess, tLookup);
    pFrame.isMainModule = pFrame.moduleBase != 0 && pFrame.moduleBase == reinterpret_cast<unsigned long long>(GetModuleHandleW(nullptr));

    if (pFrame.moduleBase)
    {
        IMAGEHLP_MODULEW64 tModule = {};
        tModule.SizeOfStruct = sizeof(tModule);

        if (SymGetModuleInfoW64(tProcess, pFrame.moduleBase, &tModule))
        {
            toNarrow(tModule.ModuleName, pFrame.module, sizeof(pFrame.module));
        }
    }

    SymbolBuffer tSymbol = {};
    tSymbol.info.SizeOfStruct = sizeof(SYMBOL_INFOW);
    tSymbol.info.MaxNameLen = 255;

    DWORD64 tDisplacement = 0;
    const bool tPlain = isPlainContext(pRaw.inlineContext);
    const BOOL tHasSymbol = tPlain ? SymFromAddrW(tProcess, tLookup, &tDisplacement, &tSymbol.info)
                                   : SymFromInlineContextW(tProcess, tLookup, pRaw.inlineContext, &tDisplacement, &tSymbol.info);

    if (tHasSymbol)
    {
        pFrame.hasSymbol = true;
        pFrame.displacement = tDisplacement + tAdjustment;
        toNarrow(tSymbol.info.Name, pFrame.function, sizeof(pFrame.function));
    }

    IMAGEHLP_LINEW64 tLine = {};
    tLine.SizeOfStruct = sizeof(tLine);
    DWORD tLineDisplacement = 0;
    const BOOL tHasLine = tPlain ? SymGetLineFromAddrW64(tProcess, tLookup, &tLineDisplacement, &tLine)
                                 : SymGetLineFromInlineContextW(tProcess, tLookup, pRaw.inlineContext, 0, &tLineDisplacement, &tLine);

    if (tHasLine && tLine.FileName)
    {
        pFrame.hasLine = true;
        pFrame.line = tLine.LineNumber;
        toNarrow(tLine.FileName, pFrame.file, sizeof(pFrame.file));
    }

    return pFrame.hasSymbol || pFrame.moduleBase != 0;
}

bool StackSymbolizer::findTopFrame(const RawFrame* pFrames, std::size_t pCount, std::size_t pLimit, std::size_t& pIndex, Frame& pFrame)
{
    if (pCount == 0)
    {
        return false;
    }

    const std::size_t tLimit = pCount < pLimit ? pCount : pLimit;
    bool tHasFallback = false;
    std::size_t tFallbackIndex = 0;

    for (std::size_t tIndex = 0; tIndex < tLimit; ++tIndex)
    {
        Frame tFrame;
        resolve(pFrames[tIndex], tFrame);

        if (CrashClassifier::isOurCode(tFrame.isMainModule && tFrame.hasLine, tFrame.file))
        {
            pIndex = tIndex;
            pFrame = tFrame;
            return true;
        }

        if (!tHasFallback && tFrame.isMainModule && tFrame.hasSymbol)
        {
            tHasFallback = true;
            tFallbackIndex = tIndex;
        }
    }

    pIndex = tHasFallback ? tFallbackIndex : 0;
    resolve(pFrames[pIndex], pFrame);
    return false;
}

std::vector<StackSymbolizer::ModuleInfo> StackSymbolizer::listModules()
{
    std::vector<ModuleInfo> tModules;
    EnumerateLoadedModulesW64(GetCurrentProcess(), onModule, &tModules);
    return tModules;
}
