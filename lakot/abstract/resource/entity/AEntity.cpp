#include "AEntity.h"

using namespace lakot;

AEntity::~AEntity()
{

}

AEntity::AEntity()
    : IEntity()
    , mType(EntityType::eNone)
{

}

const std::string& AEntity::getUUID() const
{
    return mUUID;
}

void AEntity::setUUID(const std::string& pUUID)
{
    mUUID = pUUID;
}

EntityType AEntity::getType() const
{
    return mType;
}
