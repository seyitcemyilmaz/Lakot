#ifndef LAKOT_DATACONTAINER_H
#define LAKOT_DATACONTAINER_H

#include <map>
#include <any>
#include <string>
#include <vector>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace lakot {

class DataContainer
{
public:
    virtual ~DataContainer();
    DataContainer();

    void remove(const std::string& pKey);

    bool getIsExist(const std::string& pKey) const;

    template <typename T>
    void set(const std::string& pKey, const std::vector<T>& pData)
    {
        mData[pKey] = pData;
    }

    template <typename T>
    void set(const std::string& pKey, const std::vector<T>&& pData)
    {
        mData[pKey] = std::move(pData);
    }

    template <typename T>
    void set(const std::string& pKey, const T& pData)
    {
        mData[pKey] = std::vector<T>{pData};
    }

    template <typename T>
    void set(const std::string& pKey, const T&& pData)
    {
        mData[pKey] = std::vector<T>{std::forward<const T>(pData)};
    }

    template <typename T>
    const std::vector<T>& get(const std::string& pKey) const
    {
        auto tIterator = mData.find(pKey);

        if (tIterator == mData.end())
        {
            throw std::runtime_error("Not found key: " + pKey);
        }

        const std::any& tData = tIterator->second;

        if (!tData.has_value())
        {
            throw std::runtime_error("A key has no value: " + pKey);
        }

        if (tData.type() != typeid(std::vector<T>))
        {
            throw std::runtime_error("Type mismatch for key: " + pKey);
        }

        return std::any_cast<const std::vector<T>&>(tData);
    }

    template <typename T>
    void append(const std::string& pKey, const std::vector<T>& pData)
    {
        auto tIterator = mData.find(pKey);

        if (tIterator != mData.end())
        {
            std::any& tData = tIterator->second;

            if (!tData.has_value())
            {
                throw std::runtime_error("A key has no value: " + pKey);
            }

            if (tData.type() != typeid(std::vector<T>))
            {
                throw std::runtime_error("Type mismatch for key: " + pKey);
            }

            auto& tVector = std::any_cast<std::vector<T>&>(tData);
            tVector.insert(tVector.end(), pData.begin(), pData.end());
        }
        else
        {
            set(pKey, pData);
        }
    }

    template <typename T>
    void append(const std::string& pKey, const std::vector<T>&& pData)
    {
        auto tIterator = mData.find(pKey);

        if (tIterator != mData.end())
        {
            std::any& tData = tIterator->second;

            if (!tData.has_value())
            {
                throw std::runtime_error("A key has no value: " + pKey);
            }

            if (tData.type() != typeid(std::vector<T>))
            {
                throw std::runtime_error("Type mismatch for key: " + pKey);
            }

            auto& tVector = std::any_cast<std::vector<T>&>(tData);
            tVector.insert(tVector.end(),
                           std::make_move_iterator(pData.begin()),
                           std::make_move_iterator(pData.end()));
        }
        else
        {
            set(pKey, std::forward<const std::vector<T>>(pData));
        }
    }

    template <typename T>
    void append(const std::string& pKey, const T& pData)
    {
        auto tIterator = mData.find(pKey);

        if (tIterator != mData.end())
        {
            std::any& tData = tIterator->second;

            if (!tData.has_value())
            {
                throw std::runtime_error("A key has no value: " + pKey);
            }

            if (tData.type() != typeid(std::vector<T>))
            {
                throw std::runtime_error("Type mismatch for key: " + pKey);
            }

            auto& tVector = std::any_cast<std::vector<T>&>(tData);
            tVector.push_back(pData);
        }
        else
        {
            set(pKey, pData);
        }
    }

    template <typename T>
    void append(const std::string& pKey, const T&& pData)
    {
        spdlog::info("ben geldim aq");
        auto tIterator = mData.find(pKey);

        if (tIterator != mData.end())
        {
            std::any& tData = tIterator->second;

            if (!tData.has_value())
            {
                throw std::runtime_error("A key has no value: " + pKey);
            }

            if (tData.type() != typeid(std::vector<T>))
            {
                throw std::runtime_error("Type mismatch for key: " + pKey);
            }

            auto& tVector = std::any_cast<std::vector<T>&>(tData);
            tVector.emplace_back(std::move(pData));
        }
        else
        {
            set(pKey, std::forward<const T>(pData));
        }
    }

private:
    std::map<std::string, std::any> mData;
};

}

#endif
