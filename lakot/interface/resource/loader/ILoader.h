#ifndef LAKOT_ILOADER_H
#define LAKOT_ILOADER_H

#include <lakot/utilities/Type.h>

#include "../../engine/IObject.h"

namespace lakot {

class ILoader : virtual public IObject
{
public:
    virtual ~ILoader() = default;
    ILoader() = default;

    virtual IObject* load() = 0;

    virtual LoaderType getType() const = 0;
};

}

#endif
