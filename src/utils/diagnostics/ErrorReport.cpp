#include "diagnostics/ErrorReport.h"

#include "diagnostics/CrashClassifier.h"
#include "diagnostics/CrashHandler.h"
#include "diagnostics/ThreadName.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <syncstream>
#include <typeinfo>

#if defined(_WIN32)
#include "diagnostics/ErrorReportWorker.h"
#include "diagnostics/ExceptionTrace.h"
#include "diagnostics/StackSymbolizer.h"

#include <windows.h>
#else
#include <unistd.h>
#include <sys/syscall.h>
#endif

using namespace lakot;

namespace
{
    constexpr int kMaxSymbolizedReports = 10;
    constexpr long long kRateWindowMilliseconds = 10000;
    constexpr std::size_t kMaxText = 300;

    std::atomic<long long> sWindowStart{0};
    std::atomic<int> sWindowCount{0};

    struct ThrowSite
    {
        char file[260];
        unsigned line;
        char function[300];
    };

    struct Message
    {
        char type[160];
        char context[kMaxText + 1];
        char what[kMaxText + 1];
        char catchFile[260];
        unsigned catchLine;
        char catchFunction[300];
        char threadName[32];
        unsigned long threadId;
    };

    unsigned long currentThreadId()
    {
#if defined(_WIN32)
        return GetCurrentThreadId();
#else
        return static_cast<unsigned long>(syscall(SYS_gettid));
#endif
    }

    bool isSymbolizationAllowed()
    {
        const long long tNow = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        long long tStart = sWindowStart.load();

        if (tNow - tStart > kRateWindowMilliseconds && sWindowStart.compare_exchange_strong(tStart, tNow))
        {
            sWindowCount.store(0);
        }

        return sWindowCount.fetch_add(1) < kMaxSymbolizedReports;
    }

    std::string_view stripKeyword(std::string_view pName)
    {
        for (std::string_view tPrefix : {std::string_view("class "), std::string_view("struct ")})
        {
            if (pName.substr(0, tPrefix.size()) == tPrefix)
            {
                pName.remove_prefix(tPrefix.size());
            }
        }

        return pName;
    }

    void printLine(const Message& pMessage, const ThrowSite* pSite)
    {
        char tDescription[kMaxText + 1];

        if (pSite)
        {
            std::snprintf(tDescription, sizeof(tDescription), "[%s] %s what=\"%s\" caught at %s:%u thrown", pMessage.context, pMessage.type,
                          pMessage.what, pMessage.catchFile, pMessage.catchLine);
        }
        else
        {
            std::snprintf(tDescription, sizeof(tDescription), "[%s] %s what=\"%s\" caught", pMessage.context, pMessage.type, pMessage.what);
        }

        char tLineText[1200];
        CrashClassifier::formatLine(tLineText, sizeof(tLineText), "ERROR", CrashHandler::getExecutableName(), "caught-error", tDescription,
                                    pSite ? pSite->file : pMessage.catchFile, pSite ? pSite->line : pMessage.catchLine,
                                    pSite ? pSite->function : pMessage.catchFunction, pMessage.threadName, pMessage.threadId);

        std::osyncstream(std::cerr) << tLineText << std::endl;
    }

#if defined(_WIN32)
    bool symbolizeThrowSite(const ExceptionTrace::Record& pRecord, ThrowSite& pSite)
    {
        StackSymbolizer tSymbolizer(StackSymbolizer::kWaitForever);
        StackSymbolizer::RawFrame tFrames[ExceptionTrace::kMaxFrames * 4];
        const std::size_t tCount = tSymbolizer.expandReturnAddresses(pRecord.frames, pRecord.frameCount, tFrames, std::size(tFrames));

        StackSymbolizer::Frame tTop;
        std::size_t tIndex = 0;

        if (!tSymbolizer.findTopFrame(tFrames, tCount, tCount, tIndex, tTop) || !tTop.hasLine)
        {
            return false;
        }

        CrashClassifier::toSourceRelative(tTop.file, pSite.file, sizeof(pSite.file));
        pSite.line = tTop.line;
        std::snprintf(pSite.function, sizeof(pSite.function), "%s", tTop.function);
        return true;
    }

    bool postSymbolization(const Message& pMessage, const std::exception* pException, bool pIsUnknown)
    {
        const ExceptionTrace::Record& tCurrent = ExceptionTrace::current();
        const bool tIsMatch = tCurrent.sequence != 0 && tCurrent.frameCount > 0
                              && (pIsUnknown || ExceptionTrace::containsObject(tCurrent, pException));

        if (!tIsMatch || !isSymbolizationAllowed())
        {
            return false;
        }

        const ExceptionTrace::Record tRecord = tCurrent;

        return ErrorReportWorker::post([pMessage, tRecord]()
        {
            ThrowSite tSite = {};
            const bool tIsKnown = symbolizeThrowSite(tRecord, tSite);
            printLine(pMessage, tIsKnown ? &tSite : nullptr);
        });
    }

    void describeFromRecord(char* pType, std::size_t pTypeCapacity)
    {
        const ExceptionTrace::Record& tRecord = ExceptionTrace::current();

        if (tRecord.sequence != 0)
        {
            char tIgnored[8];
            ExceptionTrace::describe(tRecord.object, tRecord.throwInfo, tRecord.imageBase, pType, pTypeCapacity, tIgnored, sizeof(tIgnored));
        }
    }
#else
    bool postSymbolization(const Message&, const std::exception*, bool)
    {
        return false;
    }

    void describeFromRecord(char*, std::size_t)
    {
    }
#endif

    void emit(std::string_view pContext, const char* pType, const char* pWhat, const std::source_location& pLocation,
              const std::exception* pException)
    {
        Message tMessage = {};
        std::snprintf(tMessage.type, sizeof(tMessage.type), "%s", pType);

        if (!pException)
        {
            describeFromRecord(tMessage.type, sizeof(tMessage.type));
        }

        std::snprintf(tMessage.context, sizeof(tMessage.context), "%.*s", static_cast<int>(pContext.size()), pContext.data());
        std::snprintf(tMessage.what, sizeof(tMessage.what), "%s", pWhat ? pWhat : "<unknown>");
        CrashClassifier::sanitize(tMessage.context, 60);
        CrashClassifier::sanitize(tMessage.what, 150);
        CrashClassifier::sanitize(tMessage.type, sizeof(tMessage.type) - 1);

        CrashClassifier::toSourceRelative(pLocation.file_name(), tMessage.catchFile, sizeof(tMessage.catchFile));
        tMessage.catchLine = static_cast<unsigned>(pLocation.line());
        std::snprintf(tMessage.catchFunction, sizeof(tMessage.catchFunction), "%s", pLocation.function_name());
        std::snprintf(tMessage.threadName, sizeof(tMessage.threadName), "%s", ThreadName::get());
        tMessage.threadId = currentThreadId();

        if (!postSymbolization(tMessage, pException, pException == nullptr))
        {
            printLine(tMessage, nullptr);
        }
    }
}

void ErrorReport::report(std::string_view pContext, const std::exception& pException, std::source_location pLocation)
{
    const std::string_view tType = stripKeyword(typeid(pException).name());
    char tTypeText[160];
    std::snprintf(tTypeText, sizeof(tTypeText), "%.*s", static_cast<int>(tType.size()), tType.data());

    emit(pContext, tTypeText, pException.what(), pLocation, &pException);
}

void ErrorReport::reportUnknown(std::string_view pContext, std::source_location pLocation)
{
    emit(pContext, "unknown", nullptr, pLocation, nullptr);
}
