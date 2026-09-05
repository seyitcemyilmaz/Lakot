#include "MapCatalog.h"

#include <cmath>
#include <vector>

using namespace lakot;

namespace
{
    const std::vector<PortalDefinition>& getPortals()
    {
        static const std::vector<PortalDefinition> tPortals =
        {
            // Town -> Forest
            {
                services::world::MAP_TOWN,
                15.0f, 0.0f, 2.0f,
                services::world::MAP_FOREST,
                0.0f, 2.0f, 10.0f, 0.0f
            },
            // Forest -> Town
            {
                services::world::MAP_FOREST,
                15.0f, 0.0f, 2.0f,
                services::world::MAP_TOWN,
                0.0f, 2.0f, 10.0f, 0.0f
            }
        };

        return tPortals;
    }
}

std::optional<PortalDefinition> MapCatalog::findPortalAt(services::world::MapId pMapId, float pX, float pZ)
{
    for (const auto& tPortal : getPortals())
    {
        if (tPortal.sourceMapId != pMapId)
        {
            continue;
        }

        float tDx = pX - tPortal.triggerX;
        float tDz = pZ - tPortal.triggerZ;

        float tHorizontalDistanceSq = tDx * tDx + tDz * tDz;

        if (tHorizontalDistanceSq <= tPortal.triggerRadius * tPortal.triggerRadius)
        {
            return tPortal;
        }
    }

    return std::nullopt;
}
