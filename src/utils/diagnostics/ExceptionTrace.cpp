#include "diagnostics/ExceptionTrace.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <exception>

#include <windows.h>

using namespace lakot;

namespace
{
    constexpr DWORD kCppExceptionCode = 0xE06D7363;
    constexpr ULONG_PTR kMagicFirst = 0x19930520;
    constexpr ULONG_PTR kMagicLast = 0x19930522;

    struct PointerDisplacement
    {
        int memberDisplacement;
        int vbtableDisplacement;
        int vdisplacement;
    };

    struct CatchableType
    {
        unsigned properties;
        int typeDescriptor;
        PointerDisplacement thisDisplacement;
        int sizeOrOffset;
        int copyFunction;
    };

    struct CatchableTypeArray
    {
        int count;
        int types[1];
    };

    struct ThrowInfo
    {
        unsigned attributes;
        int unwind;
        int forwardCompat;
        int catchableTypeArray;
    };

    struct TypeDescriptor
    {
        const void* vftable;
        void* spare;
        char name[1];
    };

    std::atomic<unsigned long long> sSequence{0};

    thread_local ExceptionTrace::Record sRecord;

    template <typename T>
    const T* resolve(unsigned long long pImageBase, int pOffset)
    {
        return reinterpret_cast<const T*>(pImageBase + static_cast<unsigned long long>(static_cast<unsigned>(pOffset)));
    }

    void copyBounded(char* pOut, std::size_t pCapacity, const char* pText)
    {
        std::size_t tIndex = 0;

        while (pText[tIndex] && tIndex + 1 < pCapacity)
        {
            pOut[tIndex] = pText[tIndex];
            ++tIndex;
        }

        pOut[tIndex] = '\0';
    }

    void demangle(const char* pRaw, char* pOut, std::size_t pCapacity)
    {
        const bool tIsPlain = std::strncmp(pRaw, ".?AV", 4) == 0 || std::strncmp(pRaw, ".?AU", 4) == 0;

        if (!tIsPlain || pRaw[4] == '?')
        {
            copyBounded(pOut, pCapacity, pRaw);
            return;
        }

        const char* tParts[8] = {};
        std::size_t tLengths[8] = {};
        std::size_t tCount = 0;
        const char* tCursor = pRaw + 4;

        while (*tCursor && *tCursor != '@' && tCount < 8)
        {
            const char* tEnd = tCursor;

            while (*tEnd && *tEnd != '@')
            {
                ++tEnd;
            }

            tParts[tCount] = tCursor;
            tLengths[tCount] = static_cast<std::size_t>(tEnd - tCursor);
            ++tCount;
            tCursor = *tEnd == '@' ? tEnd + 1 : tEnd;
        }

        std::size_t tWritten = 0;

        for (std::size_t tIndex = tCount; tIndex > 0 && tWritten + 1 < pCapacity; --tIndex)
        {
            if (tIndex != tCount)
            {
                for (int tColon = 0; tColon < 2 && tWritten + 1 < pCapacity; ++tColon)
                {
                    pOut[tWritten++] = ':';
                }
            }

            for (std::size_t tChar = 0; tChar < tLengths[tIndex - 1] && tWritten + 1 < pCapacity; ++tChar)
            {
                pOut[tWritten++] = tParts[tIndex - 1][tChar];
            }
        }

        pOut[tWritten] = '\0';
    }

    bool describeUnguarded(const void* pObject, const void* pThrowInfo, unsigned long long pImageBase,
                           char* pType, std::size_t pTypeCapacity, char* pWhat, std::size_t pWhatCapacity)
    {
        const ThrowInfo* tThrow = static_cast<const ThrowInfo*>(pThrowInfo);
        const CatchableTypeArray* tArray = resolve<CatchableTypeArray>(pImageBase, tThrow->catchableTypeArray);

        if (tArray->count <= 0)
        {
            return false;
        }

        const CatchableType* tFirst = resolve<CatchableType>(pImageBase, tArray->types[0]);
        demangle(resolve<TypeDescriptor>(pImageBase, tFirst->typeDescriptor)->name, pType, pTypeCapacity);

        copyBounded(pWhat, pWhatCapacity, "<unavailable>");

        for (int tIndex = 0; tIndex < tArray->count; ++tIndex)
        {
            const CatchableType* tCatchable = resolve<CatchableType>(pImageBase, tArray->types[tIndex]);

            if (std::strcmp(resolve<TypeDescriptor>(pImageBase, tCatchable->typeDescriptor)->name, ".?AVexception@std@@") != 0)
            {
                continue;
            }

            if (tCatchable->thisDisplacement.vbtableDisplacement < 0)
            {
                const auto* tBase = reinterpret_cast<const std::exception*>(static_cast<const char*>(pObject) + tCatchable->thisDisplacement.memberDisplacement);
                const char* tWhat = tBase->what();

                if (tWhat)
                {
                    copyBounded(pWhat, pWhatCapacity, tWhat);
                }
            }

            break;
        }

        return true;
    }

    bool describeGuarded(const void* pObject, const void* pThrowInfo, unsigned long long pImageBase,
                         char* pType, std::size_t pTypeCapacity, char* pWhat, std::size_t pWhatCapacity)
    {
        __try
        {
            return describeUnguarded(pObject, pThrowInfo, pImageBase, pType, pTypeCapacity, pWhat, pWhatCapacity);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool containsGuarded(const ExceptionTrace::Record& pRecord, const void* pAddress)
    {
        __try
        {
            const ThrowInfo* tThrow = static_cast<const ThrowInfo*>(pRecord.throwInfo);
            const CatchableTypeArray* tArray = resolve<CatchableTypeArray>(pRecord.imageBase, tThrow->catchableTypeArray);

            if (tArray->count <= 0)
            {
                return false;
            }

            const CatchableType* tFirst = resolve<CatchableType>(pRecord.imageBase, tArray->types[0]);
            const char* tBegin = static_cast<const char*>(pRecord.object);
            const char* tAddress = static_cast<const char*>(pAddress);

            return tAddress >= tBegin && tAddress < tBegin + (tFirst->sizeOrOffset > 0 ? tFirst->sizeOrOffset : 1);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    LONG CALLBACK onVectoredException(PEXCEPTION_POINTERS pPointers)
    {
        const EXCEPTION_RECORD* tRecord = pPointers->ExceptionRecord;

        if (tRecord->ExceptionCode != kCppExceptionCode || tRecord->NumberParameters < 4
            || tRecord->ExceptionInformation[0] < kMagicFirst || tRecord->ExceptionInformation[0] > kMagicLast)
        {
            return EXCEPTION_CONTINUE_SEARCH;
        }

        if (tRecord->ExceptionInformation[1] == 0 && tRecord->ExceptionInformation[2] == 0)
        {
            sRecord.isRethrow = true;
            return EXCEPTION_CONTINUE_SEARCH;
        }

        sRecord.sequence = sSequence.fetch_add(1, std::memory_order_relaxed) + 1;
        sRecord.object = reinterpret_cast<const void*>(tRecord->ExceptionInformation[1]);
        sRecord.throwInfo = reinterpret_cast<const void*>(tRecord->ExceptionInformation[2]);
        sRecord.imageBase = tRecord->ExceptionInformation[3];
        sRecord.isRethrow = false;
        sRecord.frameCount = RtlCaptureStackBackTrace(0, static_cast<DWORD>(ExceptionTrace::kMaxFrames), sRecord.frames, nullptr);

        return EXCEPTION_CONTINUE_SEARCH;
    }
}

void ExceptionTrace::install()
{
    AddVectoredExceptionHandler(1, onVectoredException);
}

const ExceptionTrace::Record& ExceptionTrace::current()
{
    return sRecord;
}

bool ExceptionTrace::describe(const void* pObject, const void* pThrowInfo, unsigned long long pImageBase,
                              char* pType, std::size_t pTypeCapacity, char* pWhat, std::size_t pWhatCapacity)
{
    if (!pObject || !pThrowInfo || pTypeCapacity == 0 || pWhatCapacity == 0)
    {
        return false;
    }

    return describeGuarded(pObject, pThrowInfo, pImageBase, pType, pTypeCapacity, pWhat, pWhatCapacity);
}

bool ExceptionTrace::containsObject(const Record& pRecord, const void* pAddress)
{
    if (!pRecord.object || !pRecord.throwInfo)
    {
        return false;
    }

    return containsGuarded(pRecord, pAddress);
}
