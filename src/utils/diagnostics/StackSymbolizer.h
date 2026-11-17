#ifndef LAKOT_STACKSYMBOLIZER_H
#define LAKOT_STACKSYMBOLIZER_H

#include <cstddef>
#include <string>
#include <vector>

#include <windows.h>

namespace lakot {

class StackSymbolizer
{
public:
    static constexpr unsigned kWaitForever = 0xFFFFFFFFu;

    struct RawFrame
    {
        unsigned long long address;
        unsigned inlineContext;
        bool isReturnAddress;
    };

    struct Frame
    {
        unsigned long long address;
        unsigned long long moduleBase;
        unsigned long long displacement;
        unsigned line;
        bool hasLine;
        bool hasSymbol;
        bool isMainModule;
        char module[64];
        char function[256];
        char file[MAX_PATH];
    };

    struct ModuleInfo
    {
        std::wstring path;
        unsigned long long base;
        unsigned size;
        const char* symbolState;
    };

    explicit StackSymbolizer(unsigned pLockWaitMilliseconds);
    ~StackSymbolizer();

    StackSymbolizer(const StackSymbolizer&) = delete;
    StackSymbolizer& operator=(const StackSymbolizer&) = delete;

    bool isLockOwned() const;

    std::size_t walk(const CONTEXT& pContext, HANDLE pThread, RawFrame* pOut, std::size_t pCapacity);

    std::size_t expandReturnAddresses(void* const* pAddresses, std::size_t pCount, RawFrame* pOut, std::size_t pCapacity);

    bool resolve(const RawFrame& pRaw, Frame& pFrame);

    bool findTopFrame(const RawFrame* pFrames, std::size_t pCount, std::size_t pLimit, std::size_t& pIndex, Frame& pFrame);

    std::vector<ModuleInfo> listModules();

private:
    void initialize();

    bool mOwnsLock;
};

}

#endif
