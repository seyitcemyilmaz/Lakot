#ifndef LAKOT_CRASHCLASSIFIER_H
#define LAKOT_CRASHCLASSIFIER_H

#include <cstddef>
#include <string_view>

namespace lakot {

class CrashClassifier
{
public:
    enum class SourceType
    {
        eSeh,
        eTerminate,
        eAbort,
        ePureCall,
        eInvalidParameter,
        eCrtReport,
        eSignal
    };

    struct Facts
    {
        SourceType source;
        unsigned long code;
        unsigned long long accessType;
        unsigned long long address;
        bool hasAddress;
        bool hasCppException;
        bool isRethrow;
        bool isUnwinding;
        bool hasAssertFrame;
        unsigned repeatCount;
        char cppType[128];
        char cppWhat[256];
        char unwindType[128];
        char message[512];
        char topFunction[256];
    };

    struct Verdict
    {
        static constexpr std::size_t kMaxCauses = 3;

        char kind[48];
        char description[320];
        char cause[kMaxCauses][320];
        unsigned causeCount;
    };

    static void classify(const Facts& pFacts, Verdict& pVerdict);

    static bool isOurCode(bool pIsMainModule, std::string_view pFile);

    static bool toSourceRelative(const char* pPath, char* pOut, std::size_t pCapacity);

    static void sanitize(char* pText, std::size_t pMaxLength);

    static std::size_t formatLine(char* pOut, std::size_t pCapacity, const char* pPrefix, const char* pExecutable,
                                  const char* pKind, const char* pDescription, const char* pFile, unsigned pLine,
                                  const char* pFunction, const char* pThreadName, unsigned long pThreadId);

private:
    CrashClassifier() = delete;
    ~CrashClassifier() = delete;
};

}

#endif
