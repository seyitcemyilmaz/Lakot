#ifndef LAKOT_VARIABLE_H
#define LAKOT_VARIABLE_H

#include <string>
#include <functional>

template <class Type>
class Variable
{
public:
    virtual ~Variable() = default;
    Variable(const std::string& pName, const Type& pValue)
        : mName(pName)
        , mValue(pValue)
    {

    }

    const std::string& getName() const noexcept
    {
        return mName;
    }

    Type& getValue() const noexcept
    {
        return mValue;
    }

    void setValue(const Type& pValue)
    {
        if (isEqual(mValue, pValue))
        {
            return;
        }

        mValue = pValue;

        if (mOnValueChangedFunction)
        {
            mOnValueChangedFunction(this);
        }
    }

private:
    std::string mName;
    Type mValue;

    std::function<void()> mOnValueChangedFunction;

    virtual bool isEqual(const Type& pValue0, const Type& pValue1)
    {
        return pValue0 == pValue1;
    }
};

template <>
inline bool Variable<double>::isEqual(const double& pValue0, const double& pValue1)
{
    return std::abs(pValue0 - pValue1) < 1e-9;
}

template <>
inline bool Variable<float>::isEqual(const float& pValue0, const float& pValue1)
{
    return std::abs(pValue0 - pValue1) < 1e-5f;
}

#endif
