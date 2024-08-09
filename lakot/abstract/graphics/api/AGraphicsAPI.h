#ifndef LAKOT_AGRAPHICSAPI_H
#define LAKOT_AGRAPHICSAPI_H

#include <lakot/interface/graphics/api/IGraphicsAPI.h>

namespace lakot {

class AGraphicsAPI : public IGraphicsAPI
{
public:
    virtual ~AGraphicsAPI() override;
    AGraphicsAPI();

    bool getIsInitialized() const override;

    GraphicsAPIType getType() const override;

    int getVersionMajor() const override;
    int getVersionMinor() const override;

    IRenderer* getRenderer() const override;

protected:
    bool mIsInitialized;

    GraphicsAPIType mType;

    int mVersionMajor;
    int mVersionMinor;

    IRenderer* mRenderer;
};

}

#endif
