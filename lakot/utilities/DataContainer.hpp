#ifndef LAKOT_DATACONTAINER_H
#define LAKOT_DATACONTAINER_H

#include <unordered_map>
#include <string>
#include <vector>
#include <any>
#include <optional>
#include <stdexcept>

namespace lakot {

class DataContainer
{
public:
    virtual ~DataContainer() = default;
    DataContainer() = default;

    void remove(const std::string& pKey)
    {
        auto tIterator = mData.find(pKey);

        if (tIterator == mData.end())
        {
            return;
        }

        mData.erase(tIterator);
    }

    bool contains(const std::string& pKey) const
    {
        return mData.find(pKey) != mData.end();
    }

    template <typename T>
    void set(const std::string& pKey, const std::vector<T>& pData)
    {
        mData.emplace(pKey, pData);
    }

    template <typename T>
    void set(const std::string& pKey, std::vector<T>&& pData)
    {
        mData[pKey] = std::move(pData);
    }

    template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, std::vector<T>>>>
    void set(const std::string& pKey, T&& pData)
    {
        mData[pKey] = std::vector<std::decay_t<T>>{std::forward<T>(pData)};
    }

    template <typename T>
    std::optional<std::reference_wrapper<const std::vector<T>>> get(const std::string& pKey) const
    {
        auto tIterator = mData.find(pKey);

        if (tIterator == mData.end())
        {
            return std::nullopt;
        }

        const std::any& tData = tIterator->second;

        if (tData.type() != typeid(std::vector<T>))
        {
            return std::nullopt;
        }

        return std::cref(std::any_cast<const std::vector<T>&>(tData));
    }

    template <typename T>
    std::vector<T>& getBuffer(const std::string& pKey)
    {
        auto tIterator = mData.find(pKey);

        if (tIterator == mData.end())
        {
            mData[pKey] = std::vector<T>();
            return std::any_cast<std::vector<T>&>(mData[pKey]);
        }

        try
        {
            return std::any_cast<std::vector<T>&>(tIterator->second);
        }
        catch (const std::bad_any_cast&)
        {
            throw std::runtime_error("Type mismatch for key: " + pKey);
        }
    }

    template <typename T>
    void append(const std::string& pKey, const std::vector<T>& pData)
    {
        auto tIterator = mData.find(pKey);

        if (tIterator == mData.end())
        {
            mData.emplace(pKey, pData);
            return;
        }

        try
        {
            auto& tVector = std::any_cast<std::vector<T>&>(tIterator->second);
            tVector.insert(tVector.end(), pData.begin(), pData.end());
        }
        catch (const std::bad_any_cast&)
        {
            throw std::runtime_error("Type mismatch for key: " + pKey);
        }
    }

    template <typename T>
    void append(const std::string& pKey, std::vector<T>&& pData)
    {
        auto tIterator = mData.find(pKey);

        if (tIterator == mData.end())
        {
            set(pKey, std::forward<std::vector<T>>(pData));
            return;
        }

        try
        {
            auto& tVector = std::any_cast<std::vector<T>&>(tIterator->second);
            tVector.insert(tVector.end(),
                           std::make_move_iterator(pData.begin()),
                           std::make_move_iterator(pData.end()));
        }
        catch (const std::bad_any_cast&)
        {
            throw std::runtime_error("Type mismatch for key: " + pKey);
        }
    }

    template <typename T>
    void append(const std::string& pKey, const T& pData)
    {
        auto tIterator = mData.find(pKey);

        if (tIterator == mData.end())
        {
            set(pKey, pData);
            return;
        }

        try
        {
            auto& tVector = std::any_cast<std::vector<T>&>(tIterator->second);
            tVector.emplace_back(pData);
        }
        catch (const std::bad_any_cast&)
        {
            throw std::runtime_error("Type mismatch for key: " + pKey);
        }
    }

    template <typename T>
    void append(const std::string& pKey, T&& pData)
    {
        auto tIterator = mData.find(pKey);

        if (tIterator == mData.end())
        {
            set(pKey, std::forward<T>(pData));
            return;
        }

        try
        {
            auto& tVector = std::any_cast<std::vector<T>&>(tIterator->second);
            tVector.emplace_back(std::forward<T>(pData));
        }
        catch (const std::bad_any_cast&)
        {
            throw std::runtime_error("Type mismatch for key: " + pKey);
        }
    }

private:
    std::unordered_map<std::string, std::any> mData;
};

}

#endif
