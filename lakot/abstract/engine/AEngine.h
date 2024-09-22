#ifndef LAKOT_AENGINE_H
#define LAKOT_AENGINE_H

#include <lakot/interface/engine/IEngine.h>
#include <lakot/interface/layer/ILayer.h>

namespace lakot {

class AEngine : virtual public IEngine
{
public:
    virtual ~AEngine() override;
    AEngine();

    virtual void run() override = 0;

protected:
    ILayer* mLayer;
};

}

#endif
