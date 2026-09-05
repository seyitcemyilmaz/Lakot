#ifndef LAKOT_SERVER_MAPCATALOG_H
#define LAKOT_SERVER_MAPCATALOG_H

#include <optional>

#include <connection.pb.h>

namespace lakot
{

struct PortalDefinition
{
    services::world::MapId sourceMapId;
    float triggerX;
    float triggerZ;
    float triggerRadius;

    services::world::MapId targetMapId;
    float targetX;
    float targetY;
    float targetZ;
    float targetYaw;
};

namespace MapCatalog
{
    // Returns the portal at pMapId whose trigger volume contains (pX, pZ),
    // if any. Horizontal-only (a vertical cylinder, not a sphere) - players
    // walk in at whatever height they're standing, they don't fly/jump to a
    // specific Y, so height shouldn't count against reaching the trigger.
    // Hand-authored/hardcoded for now - no map content pipeline yet, just
    // enough to make zone travel real.
    std::optional<PortalDefinition> findPortalAt(services::world::MapId pMapId, float pX, float pZ);
}

}

#endif
