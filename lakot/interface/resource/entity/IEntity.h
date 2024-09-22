#ifndef LAKOT_IENTITY_H
#define LAKOT_IENTITY_H

#include <lakot/utilities/Type.h>

#include "../../engine/IObject.h"

namespace lakot {

class IEntity : virtual public IObject
{
public:
    virtual ~IEntity() = default;
    IEntity() = default;

    virtual const std::string& getUUID() const = 0;
    virtual void setUUID(const std::string& pUUID) = 0;

    virtual EntityType getType() const = 0;
};

}

#endif
