#ifndef LAKOT_LOGRING_H
#define LAKOT_LOGRING_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string_view>

namespace lakot {

class LogRing
{
public:
    static constexpr std::size_t kCapacity = 65536;

    void append(std::string_view pText);

    std::size_t copyTail(char* pOut, std::size_t pCapacity, std::size_t pMaxLines, unsigned pLockWaitMilliseconds);

private:
    char mData[kCapacity] = {};
    std::atomic<std::uint64_t> mTotal{0};
    std::mutex mMutex;
};

}

#endif
