#ifndef LAKOT_AENTITY_H
#define LAKOT_AENTITY_H

#include <lakot/interface/resource/entity/IEntity.h>

namespace lakot {

class AEntity : virtual public IEntity
{
public:
    virtual ~AEntity() override;
    AEntity();

    const std::string& getUUID() const override;
    void setUUID(const std::string& pUUID) override;

    EntityType getType() const override;

protected:
    std::string mUUID;

    EntityType mType;
};

}

#endif
