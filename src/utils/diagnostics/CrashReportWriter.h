#ifndef LAKOT_CRASHREPORTWRITER_H
#define LAKOT_CRASHREPORTWRITER_H

#include "diagnostics/CrashClassifier.h"
#include "diagnostics/ExceptionTrace.h"

#include <cstddef>

#include <windows.h>

namespace lakot {

class LogRing;
class StackSymbolizer;

class CrashReportWriter
{
public:
    struct Settings
    {
        char executableName[64];
        wchar_t baseDirectory[1024];
        bool isFullDump;
        HANDLE errorHandle;
        unsigned long long executableBase;
        unsigned long executableSize;
        unsigned long linkTimestamp;
        LogRing* ring;
    };

    struct Request
    {
        CrashClassifier::SourceType source;
        EXCEPTION_RECORD exceptionRecord;
        CONTEXT context;
        unsigned long threadId;
        char threadName[32];
        ExceptionTrace::Record trace;
        bool hasCppException;
        char cppType[128];
        char cppWhat[256];
        char message[512];
        char provisional[1024];
    };

    explicit CrashReportWriter(const Settings& pSettings);

    void writeProvisional(const Request& pRequest);

    void write(Request& pRequest);

    void echoToErrorStream(const char* pLine) const;

private:
    void ensureDirectory();
    void writeDump(Request& pRequest);
    void writeFinalReports(Request& pRequest, StackSymbolizer& pSymbolizer);

    void buildPath(wchar_t* pOut, std::size_t pCapacity, const wchar_t* pFileName) const;

    const Settings& mSettings;
    wchar_t mDirectory[1200];
    bool mHasDirectory;
};

}

#endif
