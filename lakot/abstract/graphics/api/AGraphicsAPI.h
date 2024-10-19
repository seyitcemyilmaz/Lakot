#ifndef LAKOT_AGRAPHICSAPI_H
#define LAKOT_AGRAPHICSAPI_H

#include <lakot/utilities/Object.h>
#include <lakot/utilities/Type.h>

#include "../../render/ARenderer.h"

namespace lakot {

class AGraphicsAPI : public Object
{
public:
    virtual ~AGraphicsAPI() override;
    AGraphicsAPI();

    bool getIsInitialized() const;

    GraphicsAPIType getType() const;

    int getVersionMajor() const;
    int getVersionMinor() const;

    ARenderer* getRenderer() const;

protected:
    bool mIsInitialized;

    GraphicsAPIType mType;

    int mVersionMajor;
    int mVersionMinor;

    ARenderer* mRenderer;
};

}

#endif
