#ifndef LAKOT_TERRAIN_LAYER_IMAGES_H
#define LAKOT_TERRAIN_LAYER_IMAGES_H

#include "../model/Texture.h"

namespace lakot
{

struct TerrainLayerImages
{
    DecodedImage grass;
    DecodedImage dryGrass;
    DecodedImage dirt;
    DecodedImage road;
    DecodedImage rock;
    DecodedImage macro;

    bool isComplete() const
    {
        return !grass.pixels.empty() && !dryGrass.pixels.empty() && !dirt.pixels.empty()
            && !road.pixels.empty() && !rock.pixels.empty() && !macro.pixels.empty();
    }
};

}

#endif
