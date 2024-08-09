#ifndef LAKOT_GRAPHICSAPI_H
#define LAKOT_GRAPHICSAPI_H

#include <lakot/abstract/graphics/api/AGraphicsAPI.h>

namespace lakot {

class GraphicsAPI : public AGraphicsAPI
{
public:
    virtual ~GraphicsAPI() override;
    GraphicsAPI();

    static GraphicsAPI* getInstance();

protected:
    static GraphicsAPI* mInstance;
};

}

#endif
