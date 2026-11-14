#ifndef LAKOT_ASSET_CACHE_HPP
#define LAKOT_ASSET_CACHE_HPP

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace lakot
{

template <typename T>
class AssetCache
{
public:
    using Clock = std::chrono::steady_clock;

    const std::shared_ptr<T>* find(const std::string& pKey)
    {
        auto tIterator = mEntries.find(pKey);

        if (tIterator == mEntries.end())
        {
            return nullptr;
        }

        tIterator->second.lastUsed = Clock::now();
        return &tIterator->second.asset;
    }

    bool contains(const std::string& pKey) const
    {
        return mEntries.find(pKey) != mEntries.end();
    }

    void insert(const std::string& pKey, std::shared_ptr<T> pAsset)
    {
        mEntries[pKey] = Entry{ std::move(pAsset), Clock::now() };
    }

    std::vector<std::shared_ptr<T>> takeIdle(Clock::time_point pNow, std::chrono::seconds pIdle)
    {
        std::vector<std::shared_ptr<T>> tIdle;

        for (auto tIterator = mEntries.begin(); tIterator != mEntries.end();)
        {
            Entry& tEntry = tIterator->second;

            if (tEntry.asset && tEntry.asset.use_count() > 1)
            {
                tEntry.lastUsed = pNow;
            }

            if (tEntry.asset && tEntry.asset.use_count() == 1 && pNow - tEntry.lastUsed >= pIdle)
            {
                tIdle.push_back(std::move(tIterator->second.asset));
                tIterator = mEntries.erase(tIterator);
            }
            else
            {
                ++tIterator;
            }
        }

        return tIdle;
    }

    void clear()
    {
        mEntries.clear();
    }

private:
    struct Entry
    {
        std::shared_ptr<T> asset;
        Clock::time_point lastUsed;
    };

    std::unordered_map<std::string, Entry> mEntries;
};

}

#endif
