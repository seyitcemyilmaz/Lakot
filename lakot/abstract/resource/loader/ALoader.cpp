#include "ALoader.h"

using namespace lakot;

ALoader::~ALoader()
{

}

ALoader::ALoader()
    : ILoader()
    , mType(LoaderType::eNone)
{

}

LoaderType ALoader::getType() const
{
    return mType;
}
