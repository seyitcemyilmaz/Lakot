#include "diagnostics/ThreadName.h"

#include <cstddef>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

using namespace lakot;

namespace
{
    constexpr std::size_t kNameCapacity = 32;

    thread_local char sName[kNameCapacity];

#if defined(_WIN32)
    using SetThreadDescriptionFunction = HRESULT(WINAPI*)(HANDLE, PCWSTR);

    void setOsName(const char* pName)
    {
        HMODULE tKernel = GetModuleHandleW(L"kernel32.dll");
        auto tFunction = tKernel ? reinterpret_cast<SetThreadDescriptionFunction>(GetProcAddress(tKernel, "SetThreadDescription")) : nullptr;

        if (!tFunction)
        {
            return;
        }

        wchar_t tWide[kNameCapacity] = {};
        MultiByteToWideChar(CP_UTF8, 0, pName, -1, tWide, static_cast<int>(kNameCapacity));
        tFunction(GetCurrentThread(), tWide);
    }
#else
    void setOsName(const char* pName)
    {
        char tShort[16] = {};
        std::strncpy(tShort, pName, sizeof(tShort) - 1);
        pthread_setname_np(pthread_self(), tShort);
    }
#endif
}

void ThreadName::set(std::string_view pName)
{
    const std::size_t tLength = pName.size() < kNameCapacity - 1 ? pName.size() : kNameCapacity - 1;

    for (std::size_t tIndex = 0; tIndex < tLength; ++tIndex)
    {
        const char tCharacter = pName[tIndex];
        sName[tIndex] = (tCharacter <= ' ' || tCharacter == ':' || tCharacter == '(' || tCharacter == ')') ? '_' : tCharacter;
    }

    sName[tLength] = '\0';
    setOsName(sName);
}

const char* ThreadName::get()
{
    return sName[0] ? sName : "unnamed";
}
