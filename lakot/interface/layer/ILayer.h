#ifndef LAKOT_ILAYER_H
#define LAKOT_ILAYER_H

#include "../engine/IObject.h"

namespace lakot {

class ILayer : public IObject
{
public:
    virtual ~ILayer() = default;
    ILayer() = default;

    virtual void update() = 0;

    virtual bool getIsInitialized() const = 0;
};

}

#endif
