#include "DataContainer.h"

#include <spdlog/spdlog.h>

using namespace lakot;

DataContainer::~DataContainer()
{

}

DataContainer::DataContainer()
{

}

void DataContainer::remove(const std::string& pKey)
{
    auto tIterator = mData.find(pKey);

    if (tIterator == mData.end())
    {
        spdlog::warn("A key is not found in container: {0}", pKey);
        return;
    }

    mData.erase(tIterator);
}

bool DataContainer::getIsExist(const std::string& pKey) const
{
    return mData.find(pKey) != mData.end();
}

