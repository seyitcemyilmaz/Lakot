#ifndef LAKOT_EXCEPTIONTRACE_H
#define LAKOT_EXCEPTIONTRACE_H

#include <cstddef>

namespace lakot {

class ExceptionTrace
{
public:
    static constexpr std::size_t kMaxFrames = 32;

    struct Record
    {
        unsigned long long sequence;
        const void* object;
        const void* throwInfo;
        unsigned long long imageBase;
        bool isRethrow;
        unsigned frameCount;
        void* frames[kMaxFrames];
    };

    static void install();

    static const Record& current();

    static bool describe(const void* pObject, const void* pThrowInfo, unsigned long long pImageBase,
                         char* pType, std::size_t pTypeCapacity, char* pWhat, std::size_t pWhatCapacity);

    static bool containsObject(const Record& pRecord, const void* pAddress);

private:
    ExceptionTrace() = delete;
    ~ExceptionTrace() = delete;
};

}

#endif
