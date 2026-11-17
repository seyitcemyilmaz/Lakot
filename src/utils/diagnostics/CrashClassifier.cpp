#include "diagnostics/CrashClassifier.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

using namespace lakot;

namespace
{
    constexpr std::size_t kMaxDescriptionLength = 300;
    constexpr unsigned long kCppExceptionCode = 0xE06D7363;

    void format(char* pOut, std::size_t pCapacity, const char* pFormat, ...)
    {
        va_list tArguments;
        va_start(tArguments, pFormat);
        std::vsnprintf(pOut, pCapacity, pFormat, tArguments);
        va_end(tArguments);
        pOut[pCapacity - 1] = '\0';
    }

    void copyText(char* pOut, std::size_t pCapacity, const char* pText)
    {
        std::snprintf(pOut, pCapacity, "%s", pText);
    }

    void stripControls(char* pText, std::size_t pMaxLength)
    {
        std::size_t tIndex = 0;

        for (; pText[tIndex] && tIndex < pMaxLength; ++tIndex)
        {
            const unsigned char tCharacter = static_cast<unsigned char>(pText[tIndex]);

            if (tCharacter < 0x20 || tCharacter == 0x7F)
            {
                pText[tIndex] = ' ';
            }
        }

        while (tIndex > 0 && pText[tIndex - 1] == ' ')
        {
            --tIndex;
        }

        pText[tIndex] = '\0';
    }

    void describeCpp(const CrashClassifier::Facts& pFacts, char* pOut, std::size_t pCapacity)
    {
        char tType[sizeof(pFacts.cppType)];
        char tWhat[sizeof(pFacts.cppWhat)];
        copyText(tType, sizeof(tType), pFacts.cppType);
        copyText(tWhat, sizeof(tWhat), pFacts.cppWhat);
        CrashClassifier::sanitize(tType, sizeof(tType));
        CrashClassifier::sanitize(tWhat, sizeof(tWhat));
        format(pOut, pCapacity, "%s what=\"%s\"", tType, tWhat);
    }

    std::size_t normalize(const char* pPath, char* pOut, std::size_t pCapacity)
    {
        std::size_t tLength = 0;

        for (; pPath[tLength] && tLength + 1 < pCapacity; ++tLength)
        {
            char tCharacter = pPath[tLength];

            if (tCharacter == '\\')
            {
                tCharacter = '/';
            }
            else if (tCharacter >= 'A' && tCharacter <= 'Z')
            {
                tCharacter = static_cast<char>(tCharacter - 'A' + 'a');
            }

            pOut[tLength] = tCharacter;
        }

        pOut[tLength] = '\0';
        return tLength;
    }

    std::size_t sourceRootLength(char* pOut, std::size_t pCapacity)
    {
        std::size_t tLength = normalize(LAKOT_SOURCE_ROOT, pOut, pCapacity);

        while (tLength > 0 && pOut[tLength - 1] == '/')
        {
            pOut[--tLength] = '\0';
        }

        return tLength;
    }

    bool isNear(unsigned long long pAddress, unsigned long long pPattern)
    {
        return pAddress + 0x10000 >= pPattern && pAddress <= pPattern + 0x10000;
    }

    bool contains(const char* pText, const char* pNeedle)
    {
        return std::strstr(pText, pNeedle) != nullptr;
    }

    void addCause(CrashClassifier::Verdict& pVerdict, const char* pFormat, ...)
    {
        if (pVerdict.causeCount >= CrashClassifier::Verdict::kMaxCauses)
        {
            return;
        }

        va_list tArguments;
        va_start(tArguments, pFormat);
        char* tTarget = pVerdict.cause[pVerdict.causeCount++];
        std::vsnprintf(tTarget, sizeof(pVerdict.cause[0]), pFormat, tArguments);
        va_end(tArguments);
        tTarget[sizeof(pVerdict.cause[0]) - 1] = '\0';
    }

    void classifyAccessViolation(const CrashClassifier::Facts& pFacts, CrashClassifier::Verdict& pVerdict)
    {
        const char* tOperation = pFacts.accessType == 0 ? "reading" : pFacts.accessType == 1 ? "writing" : "executing";
        const unsigned long long tAddress = pFacts.address;

        copyText(pVerdict.kind, sizeof(pVerdict.kind), "access-violation");
        format(pVerdict.description, sizeof(pVerdict.description), "%s 0x%016llX", tOperation, tAddress);

        if (pFacts.accessType == 8)
        {
            addCause(pVerdict, "call through a bad function pointer or vtable (the instruction pointer jumped to 0x%016llX)", tAddress);
        }
        else if (tAddress == 0)
        {
            addCause(pVerdict, "null pointer dereference (%s address 0)", tOperation);
        }
        else if (tAddress < 0x10000)
        {
            addCause(pVerdict, "null pointer dereference at member offset 0x%llX (a member of a null object was accessed)", tAddress);
        }
        else if (tAddress == 0xFFFFFFFFFFFFFFFFull)
        {
            addCause(pVerdict, "wild or garbage pointer (non-canonical address)");
        }
        else if (isNear(tAddress, 0xDDDDDDDDull))
        {
            addCause(pVerdict, "use after free (debug heap fill 0xDD)");
        }
        else if (isNear(tAddress, 0xCDCDCDCDull))
        {
            addCause(pVerdict, "uninitialized heap memory (debug heap fill 0xCD)");
        }
        else if (isNear(tAddress, 0xCCCCCCCCull))
        {
            addCause(pVerdict, "uninitialized stack variable (debug stack fill 0xCC)");
        }
        else if (isNear(tAddress, 0xFEEEFEEEull))
        {
            addCause(pVerdict, "freed heap memory (fill 0xFEEEFEEE)");
        }
        else
        {
            addCause(pVerdict, "invalid memory address: dangling or corrupted pointer, or an index past the end of a buffer");
        }
    }

    void addCppCause(const CrashClassifier::Facts& pFacts, CrashClassifier::Verdict& pVerdict)
    {
        const char* tType = pFacts.cppType;

        if (contains(tType, "out_of_range"))
        {
            addCause(pVerdict, "%s: .at() or an index/offset out of range", tType);
        }
        else if (contains(tType, "invalid_argument"))
        {
            addCause(pVerdict, "%s: stoi/stof/stoul on non-numeric text or an invalid argument", tType);
        }
        else if (contains(tType, "bad_alloc") || contains(tType, "length_error"))
        {
            addCause(pVerdict, "%s: huge or negative size requested", tType);
        }
        else if (contains(tType, "bad_function_call"))
        {
            addCause(pVerdict, "%s: empty std::function was invoked", tType);
        }
        else if (contains(tType, "system_error"))
        {
            addCause(pVerdict, "%s: thread or mutex misuse", tType);
        }
        else if (contains(tType, "pqxx"))
        {
            addCause(pVerdict, "%s: database error", tType);
        }
        else if (contains(tType, "sol::error"))
        {
            addCause(pVerdict, "%s: Lua script error", tType);
        }
        else
        {
            addCause(pVerdict, "uncaught %s with no matching catch on this thread", tType);
        }
    }

    void classifyTerminate(const CrashClassifier::Facts& pFacts, CrashClassifier::Verdict& pVerdict)
    {
        if (pFacts.hasCppException && !pFacts.isRethrow)
        {
            copyText(pVerdict.kind, sizeof(pVerdict.kind), "uncaught-exception");
            describeCpp(pFacts, pVerdict.description, sizeof(pVerdict.description));
            addCppCause(pFacts, pVerdict);
            return;
        }

        copyText(pVerdict.kind, sizeof(pVerdict.kind), "terminate");

        if (pFacts.hasCppException)
        {
            copyText(pVerdict.description, sizeof(pVerdict.description), "rethrow (throw;) reached std::terminate");
            addCause(pVerdict, "throw; executed outside a catch block, or the rethrown exception had no handler");
            return;
        }

        copyText(pVerdict.description, sizeof(pVerdict.description), "std::terminate called with no exception in flight");
        addCause(pVerdict, "a joinable std::thread was destroyed, throw; outside a catch, or a throw inside a noexcept function or destructor");
    }

    void classifySeh(const CrashClassifier::Facts& pFacts, CrashClassifier::Verdict& pVerdict)
    {
        switch (pFacts.code)
        {
        case 0xC0000005:
            classifyAccessViolation(pFacts, pVerdict);
            break;
        case 0xC00000FD:
            copyText(pVerdict.kind, sizeof(pVerdict.kind), "stack-overflow");
            copyText(pVerdict.description, sizeof(pVerdict.description), "thread stack exhausted");

            if (pFacts.repeatCount > 8 && pFacts.topFunction[0])
            {
                addCause(pVerdict, "%s repeats %u times on the stack: unbounded recursion", pFacts.topFunction, pFacts.repeatCount);
            }
            else
            {
                addCause(pVerdict, "very large stack allocation or deep call chain (no single function repeats)");
            }
            break;
        case 0xC0000094:
            copyText(pVerdict.kind, sizeof(pVerdict.kind), "divide-by-zero");
            copyText(pVerdict.description, sizeof(pVerdict.description), "integer division by zero");
            addCause(pVerdict, "check the divisor before the integer division or modulo");
            break;
        case 0xC0000095:
            copyText(pVerdict.kind, sizeof(pVerdict.kind), "int-overflow");
            copyText(pVerdict.description, sizeof(pVerdict.description), "integer overflow");
            addCause(pVerdict, "integer division overflow (INT_MIN / -1)");
            break;
        case 0xC000008E:
        case 0xC000008F:
        case 0xC0000090:
        case 0xC0000091:
        case 0xC0000092:
        case 0xC0000093:
            copyText(pVerdict.kind, sizeof(pVerdict.kind), "float-error");
            format(pVerdict.description, sizeof(pVerdict.description), "floating point exception 0x%08lX", pFacts.code);
            addCause(pVerdict, "floating point trap: division by zero, invalid operation or overflow with unmasked FP exceptions");
            break;
        case 0xC000001D:
            copyText(pVerdict.kind, sizeof(pVerdict.kind), "illegal-instruction");
            copyText(pVerdict.description, sizeof(pVerdict.description), "illegal instruction");
            addCause(pVerdict, "executing data or corrupted code, an unreachable path (__assume / ud2) or an unsupported CPU instruction");
            break;
        case 0xC0000096:
            copyText(pVerdict.kind, sizeof(pVerdict.kind), "privileged-instruction");
            copyText(pVerdict.description, sizeof(pVerdict.description), "privileged instruction");
            addCause(pVerdict, "executing data or corrupted code");
            break;
        case 0xC0000006:
            copyText(pVerdict.kind, sizeof(pVerdict.kind), "in-page-error");
            format(pVerdict.description, sizeof(pVerdict.description), "in page error at 0x%016llX", pFacts.address);
            addCause(pVerdict, "memory-mapped file or image page could not be read (file removed or I/O error)");
            break;
        case 0x80000002:
            copyText(pVerdict.kind, sizeof(pVerdict.kind), "datatype-misalignment");
            copyText(pVerdict.description, sizeof(pVerdict.description), "datatype misalignment");
            addCause(pVerdict, "unaligned access to data that requires alignment");
            break;
        case 0xC000008C:
            copyText(pVerdict.kind, sizeof(pVerdict.kind), "array-bounds");
            copyText(pVerdict.description, sizeof(pVerdict.description), "array bounds exceeded");
            addCause(pVerdict, "array index out of range (hardware bound check)");
            break;
        case 0xC0000374:
            copyText(pVerdict.kind, sizeof(pVerdict.kind), "heap-corruption");
            copyText(pVerdict.description, sizeof(pVerdict.description), "heap corruption detected");
            addCause(pVerdict, "buffer overrun or double delete; the crash site may be far from the actual bug");
            break;
        case 0x80000003:
            copyText(pVerdict.kind, sizeof(pVerdict.kind), "breakpoint");
            copyText(pVerdict.description, sizeof(pVerdict.description), "breakpoint instruction executed");
            addCause(pVerdict, "__debugbreak() or a failed runtime check without a debugger attached");
            break;
        case kCppExceptionCode:
            copyText(pVerdict.kind, sizeof(pVerdict.kind), "uncaught-exception");
            describeCpp(pFacts, pVerdict.description, sizeof(pVerdict.description));
            addCppCause(pFacts, pVerdict);
            break;
        default:
            format(pVerdict.kind, sizeof(pVerdict.kind), "seh-0x%08lX", pFacts.code);
            format(pVerdict.description, sizeof(pVerdict.description), "unrecognized structured exception 0x%08lX", pFacts.code);
            addCause(pVerdict, "see the NTSTATUS code in report.txt");
            break;
        }
    }
}

void CrashClassifier::classify(const Facts& pFacts, Verdict& pVerdict)
{
    std::memset(&pVerdict, 0, sizeof(pVerdict));

    switch (pFacts.source)
    {
    case SourceType::eSeh:
        classifySeh(pFacts, pVerdict);
        break;
    case SourceType::eTerminate:
        classifyTerminate(pFacts, pVerdict);
        break;
    case SourceType::eAbort:
        if (pFacts.hasAssertFrame)
        {
            copyText(pVerdict.kind, sizeof(pVerdict.kind), "assert");
            copyText(pVerdict.description, sizeof(pVerdict.description), pFacts.message[0] ? pFacts.message : "assert() failed (the expression is on stderr above)");
            addCause(pVerdict, "an assert() condition was false; the message above is the failed expression");
        }
        else
        {
            copyText(pVerdict.kind, sizeof(pVerdict.kind), "abort");
            copyText(pVerdict.description, sizeof(pVerdict.description), "abort() called");
            addCause(pVerdict, "abort() was called directly, or a runtime library failure ended in abort");
        }
        break;
    case SourceType::ePureCall:
        copyText(pVerdict.kind, sizeof(pVerdict.kind), "pure-virtual-call");
        copyText(pVerdict.description, sizeof(pVerdict.description), "pure virtual function called");
        addCause(pVerdict, "virtual call during construction or destruction, or on an object that was already deleted");
        break;
    case SourceType::eInvalidParameter:
        copyText(pVerdict.kind, sizeof(pVerdict.kind), "invalid-parameter");
        format(pVerdict.description, sizeof(pVerdict.description), "CRT invalid parameter: %s", pFacts.message[0] ? pFacts.message : "unknown");
        addCause(pVerdict, "bad argument to a CRT function or a failed STL debug check");
        break;
    case SourceType::eCrtReport:
        copyText(pVerdict.kind, sizeof(pVerdict.kind), pFacts.hasAssertFrame ? "assert" : "crt-assert");
        copyText(pVerdict.description, sizeof(pVerdict.description), pFacts.message[0] ? pFacts.message : "CRT assertion failed");
        addCause(pVerdict, "failed assert() or STL debug check (iterator or index misuse)");
        break;
    case SourceType::eSignal:
        format(pVerdict.kind, sizeof(pVerdict.kind), "seh-0x%08lX", pFacts.code);
        copyText(pVerdict.description, sizeof(pVerdict.description), pFacts.message);
        break;
    }

    if (pFacts.isUnwinding && pFacts.source != SourceType::eTerminate)
    {
        addCause(pVerdict, "happened while unwinding %s", pFacts.unwindType[0] ? pFacts.unwindType : "a C++ exception");
    }

    stripControls(pVerdict.description, kMaxDescriptionLength);
}

bool CrashClassifier::isOurCode(bool pIsMainModule, std::string_view pFile)
{
    if (!pIsMainModule || pFile.empty())
    {
        return false;
    }

    char tFile[1024];
    char tRoot[1024];
    const std::size_t tFileLength = normalize(std::string(pFile).c_str(), tFile, sizeof(tFile));
    const std::size_t tRootLength = sourceRootLength(tRoot, sizeof(tRoot));

    if (tFileLength <= tRootLength + 5 || std::strncmp(tFile, tRoot, tRootLength) != 0 || std::strncmp(tFile + tRootLength, "/src/", 5) != 0)
    {
        return false;
    }

    return std::strstr(tFile, "/src/utils/diagnostics/") == nullptr;
}

bool CrashClassifier::toSourceRelative(const char* pPath, char* pOut, std::size_t pCapacity)
{
    if (!pPath || !pPath[0])
    {
        copyText(pOut, pCapacity, "?");
        return false;
    }

    char tNormalized[1024];
    char tRoot[1024];
    normalize(pPath, tNormalized, sizeof(tNormalized));
    const std::size_t tRootLength = sourceRootLength(tRoot, sizeof(tRoot));
    const bool tIsInside = std::strncmp(tNormalized, tRoot, tRootLength) == 0 && tNormalized[tRootLength] == '/';

    const char* tStart = pPath;

    if (tIsInside)
    {
        tStart = pPath + tRootLength + 1;
    }
    else
    {
        for (const char* tCursor = pPath; *tCursor; ++tCursor)
        {
            if (*tCursor == '\\' || *tCursor == '/')
            {
                tStart = tCursor + 1;
            }
        }
    }

    std::size_t tLength = 0;

    for (; tStart[tLength] && tLength + 1 < pCapacity; ++tLength)
    {
        char tCharacter = tStart[tLength];
        pOut[tLength] = tCharacter == '\\' ? '/' : (tCharacter <= ' ' ? '_' : tCharacter);
    }

    pOut[tLength] = '\0';

    if (tLength == 0)
    {
        copyText(pOut, pCapacity, "?");
    }

    return tIsInside;
}

void CrashClassifier::sanitize(char* pText, std::size_t pMaxLength)
{
    std::size_t tIndex = 0;

    for (; pText[tIndex] && tIndex < pMaxLength; ++tIndex)
    {
        const unsigned char tCharacter = static_cast<unsigned char>(pText[tIndex]);

        if (tCharacter < 0x20 || tCharacter == 0x7F)
        {
            pText[tIndex] = ' ';
        }
        else if (tCharacter == '"')
        {
            pText[tIndex] = '\'';
        }
    }

    pText[tIndex] = '\0';
}

std::size_t CrashClassifier::formatLine(char* pOut, std::size_t pCapacity, const char* pPrefix, const char* pExecutable,
                                        const char* pKind, const char* pDescription, const char* pFile, unsigned pLine,
                                        const char* pFunction, const char* pThreadName, unsigned long pThreadId)
{
    char tDescription[kMaxDescriptionLength + 1];
    char tFunction[260];
    std::snprintf(tDescription, sizeof(tDescription), "%s", pDescription);
    std::snprintf(tFunction, sizeof(tFunction), "%s", pFunction);
    stripControls(tDescription, kMaxDescriptionLength);
    stripControls(tFunction, sizeof(tFunction) - 1);

    const int tWritten = std::snprintf(pOut, pCapacity, "%s %s %s %s at %s:%u in %s (thread %s:%lu)", pPrefix, pExecutable, pKind,
                                       tDescription, pFile, pLine, tFunction, pThreadName, pThreadId);

    if (tWritten < 0)
    {
        pOut[0] = '\0';
        return 0;
    }

    return static_cast<std::size_t>(tWritten) < pCapacity ? static_cast<std::size_t>(tWritten) : pCapacity - 1;
}
