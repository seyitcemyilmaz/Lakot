#ifndef LAKOT_ALAYER_H
#define LAKOT_ALAYER_H

#include <lakot/interface/layer/ILayer.h>

namespace lakot {

class ALayer : public ILayer
{
public:
    virtual ~ALayer() override;
    ALayer();

    virtual void update() override = 0;

    bool getIsInitialized() const override;

protected:
    bool mIsInitialized;
};

}

#endif
