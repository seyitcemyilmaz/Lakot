#ifndef LAKOT_AENTITY_H
#define LAKOT_AENTITY_H

#include <lakot/utilities/Object.h>
#include <lakot/utilities/Type.h>

namespace lakot {

class AEntity : public Object
{
public:
    virtual ~AEntity() override;
    AEntity();

    const std::string& getUUID() const noexcept;
    void setUUID(const std::string& pUUID) noexcept;

    EntityType getType() const noexcept;

protected:
    std::string mUUID;

    EntityType mType;
};

}

#endif
