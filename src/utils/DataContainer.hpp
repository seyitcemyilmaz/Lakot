#ifndef LAKOT_DATACONTAINER_HPP
#define LAKOT_DATACONTAINER_HPP

#include <any>
#include <vector>
#include <string>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <functional>
#include <utility>
#include <unordered_map>

namespace lakot
{

template<typename T> struct is_vector : std::false_type {};
template<typename T, typename A> struct is_vector<std::vector<T, A>> : std::true_type {};
template<typename T> inline constexpr bool is_vector_v = is_vector<T>::value;

class DataContainer
{
public:
    virtual ~DataContainer() = default;
    DataContainer() = default;

    void remove(const std::string& pKey)
    {
        mData.erase(pKey);
    }

    bool contains(const std::string& pKey) const
    {
        return mData.find(pKey) != mData.end();
    }

    template <typename T>
    void set(const std::string& pKey, const std::vector<T>& pData)
    {
        mData.insert_or_assign(pKey, pData);
    }

    template <typename T>
    void set(const std::string& pKey, std::vector<T>&& pData)
    {
        mData.insert_or_assign(pKey, std::move(pData));
    }

    template <typename T, typename = std::enable_if_t<!is_vector_v<std::decay_t<T>>>>
    void set(const std::string& pKey, T&& pData)
    {
        std::vector<std::decay_t<T>> tVector;
        tVector.push_back(std::forward<T>(pData));
        mData.insert_or_assign(pKey, std::move(tVector));
    }

    template <typename T>
    std::optional<std::reference_wrapper<const std::vector<T>>> get(const std::string& pKey) const
    {
        auto tIterator = mData.find(pKey);

        if (tIterator == mData.end())
        {
            return std::nullopt;
        }

        const std::vector<T>* tVectorPtr = std::any_cast<std::vector<T>>(&tIterator->second);

        if (!tVectorPtr)
        {
            return std::nullopt;
        }

        return std::cref(*tVectorPtr);
    }

    template <typename T>
    std::vector<T>& getBuffer(const std::string& pKey)
    {
        auto [tIterator, tInserted] = mData.try_emplace(pKey, std::vector<T>{});

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
        this->appendImplementation<T>(pKey, pData);
    }

    template <typename T>
    void append(const std::string& pKey, std::vector<T>&& pData)
    {
        this->appendImplementation<T>(pKey, std::move(pData));
    }

    template <typename T>
    void append(const std::string& pKey, const T& pData)
    {
        std::vector<T>& tVector = getBuffer<T>(pKey);
        tVector.push_back(pData);
    }

    template <typename T>
    void append(const std::string& pKey, T&& pData)
    {
        std::vector<T>& tVector = getBuffer<T>(pKey);
        tVector.push_back(std::forward<T>(pData));
    }

private:
    std::unordered_map<std::string, std::any> mData;

    template <typename T, typename VectorType>
    void appendImplementation(const std::string& pKey, VectorType&& pData)
    {
        auto tIterator = mData.find(pKey);

        if (tIterator == mData.end())
        {
            mData.emplace(pKey, std::forward<VectorType>(pData));
            return;
        }

        try
        {
            auto& tVector = std::any_cast<std::vector<T>&>(tIterator->second);

            if constexpr (std::is_rvalue_reference_v<decltype(pData)>)
            {
                tVector.insert(tVector.end(),
                               std::make_move_iterator(pData.begin()),
                               std::make_move_iterator(pData.end()));
            }
            else
            {
                tVector.insert(tVector.end(), pData.begin(), pData.end());
            }
        }
        catch (const std::bad_any_cast&)
        {
            throw std::runtime_error("Type mismatch for key: " + pKey);
        }
    }
};

}

#endif
