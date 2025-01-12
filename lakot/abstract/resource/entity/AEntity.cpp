#include "AEntity.h"

using namespace lakot;

AEntity::~AEntity()
{

}

AEntity::AEntity()
    : Object()
    , mType(EntityType::eUndefined)
{

}

const std::string& AEntity::getUUID() const noexcept
{
    return mUUID;
}

void AEntity::setUUID(const std::string& pUUID) noexcept
{
    mUUID = pUUID;
}

EntityType AEntity::getType() const noexcept
{
    return mType;
}
