#ifndef LAKOT_LAYER_H
#define LAKOT_LAYER_H

#include <lakot/abstract/layer/ALayer.h>

namespace lakot {

class Layer : public ALayer
{
public:
    virtual ~Layer() override;
    Layer();

    static Layer* getInstance();

    void initialize() override;
    void deinitialize() override;

    void update() override;

protected:
    static Layer* mInstance;


};

}

#endif
