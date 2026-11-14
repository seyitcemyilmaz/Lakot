#ifndef LAKOT_ROLLING_AVERAGE_H
#define LAKOT_ROLLING_AVERAGE_H

#include <algorithm>
#include <array>
#include <cstddef>

namespace lakot
{

template <size_t SampleCount>
class RollingAverage
{
public:
    void add(float pValue)
    {
        mSamples[mIndex] = pValue;
        mIndex = (mIndex + 1) % SampleCount;
        mCount = std::min(mCount + 1, SampleCount);
    }

    float get() const
    {
        if (mCount == 0)
        {
            return 0.0f;
        }

        float tSum = 0.0f;

        for (size_t tIndex = 0; tIndex < mCount; tIndex++)
        {
            tSum += mSamples[tIndex];
        }

        return tSum / static_cast<float>(mCount);
    }

private:
    std::array<float, SampleCount> mSamples{};
    size_t mIndex{0};
    size_t mCount{0};
};

}

#endif
