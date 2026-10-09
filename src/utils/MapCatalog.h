#ifndef LAKOT_MAPCATALOG_H
#define LAKOT_MAPCATALOG_H

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "MapData.h"

namespace lakot
{

struct KingdomInfo
{
    uint32_t id = 0;
    std::string name;
    std::string description;
    std::string color;          // "#RRGGBB"
    uint32_t startMapId = 0;
};

// Every map and kingdom, loaded once from the data directory:
//
//   data/kingdoms.json          the kingdoms and where each one starts
//   data/maps/<folder>/         one MapData per folder
//
// Read-only after load(), so it can be shared across threads without a lock.
class MapCatalog
{
public:
    static constexpr uint32_t kKingdomCount = 3;

    static bool isValidKingdom(uint32_t pKingdom);

    // false on failure, with the reason in pError.
    bool load(const std::string& pDataDirectory, std::string& pError);

    const MapData* getMap(uint32_t pMapId) const;
    std::vector<uint32_t> getMapIds() const;

    const std::vector<KingdomInfo>& getKingdoms() const;
    const KingdomInfo* getKingdom(uint32_t pKingdom) const;

    // The kingdom's own starting map, or the default one while that kingdom
    // has no map of its own yet.
    const MapData* getStartMap(uint32_t pKingdom) const;

    // The portal on pMapId whose trigger contains (pX, pZ), if any.
    // Horizontal only - a vertical cylinder.
    std::optional<MapPortal> findPortalAt(uint32_t pMapId, float pX, float pZ) const;

private:
    std::map<uint32_t, std::unique_ptr<MapData>> mMaps;
    std::vector<KingdomInfo> mKingdoms;
    uint32_t mDefaultStartMapId = 0;
};

}

#endif
