#ifndef LAKOT_MAPDATA_H
#define LAKOT_MAPDATA_H

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace lakot
{

// A decoration or building standing on the ground. Drawn with its model if
// it names one (data/models/<model>.glb), otherwise as a box. Either way
// sizeX/Y/Z is its box, used for picking and the map picture.
struct MapObject
{
    std::string type;
    std::string model;
    float yaw = 0.0f;                    // radians, about +y
    float scale = 1.0f;                  // uniform, applied to the model
    float stretch = 1.0f;                // extra scale along the model's own x
    float x = 0.0f;
    float z = 0.0f;
    float sizeX = 1.0f;
    float sizeY = 1.0f;
    float sizeZ = 1.0f;
    float elevation = 0.0f;              // above the ground under (x, z)
    std::array<float, 3> color{1.0f, 1.0f, 1.0f};
    bool isBlocking = false;             // already baked into block.png

    // Not drawn: a building's collision/picking box, whose look comes from
    // its model pieces. Still used for picking and the map picture.
    bool isHidden = false;
};

struct MapPortal
{
    float triggerX = 0.0f;
    float triggerZ = 0.0f;
    float triggerRadius = 0.0f;

    uint32_t targetMapId = 0;
    float targetX = 0.0f;
    float targetZ = 0.0f;
    float targetYaw = 0.0f;
};

struct MapNpc
{
    std::string name;
    std::string model;
    std::string role;
    float x = 0.0f;
    float z = 0.0f;
    float yaw = 0.0f;
};

struct MapMonsterSpawn
{
    uint32_t monsterId = 0;
    float x = 0.0f;
    float z = 0.0f;
    float radius = 0.0f;
    uint32_t count = 0;
    uint32_t minLevel = 0;
    uint32_t maxLevel = 0;
};

struct MapSafeZone
{
    std::string name;
    std::vector<std::array<float, 2>> points;
};

struct MapSpawn
{
    float x = 0.0f;
    float z = 0.0f;
    float yaw = 0.0f;
};

// One map, loaded from its folder under data/maps/ - the same data on the
// server (walkability, spawns, portals) and the client (terrain, objects).
//
//   map.json    id, name, kingdom, world size, height range, water level,
//               spawn, portals, objects
//   height.png  16-bit greyscale, square; vertices spread evenly over the
//               world, which is centred on the origin
//   ground.png  RGB ground colour per height vertex
//   block.png   8-bit, square; >= 128 means the cell cannot be walked on
//   roads.png   optional, 8-bit, same size as block.png; >= 128 marks a road
//               that walkers prefer
class MapData
{
public:
    // nullptr on failure, with the reason in pError.
    static std::unique_ptr<MapData> load(const std::string& pDirectory, std::string& pError);

    uint32_t getId() const;
    const std::string& getName() const;
    uint32_t getKingdom() const;          // 0 = shared by every kingdom

    float getWorldSize() const;
    float getWaterLevel() const;

    const MapSpawn& getSpawn() const;
    const std::vector<MapPortal>& getPortals() const;
    const std::vector<MapObject>& getObjects() const;
    const std::vector<MapNpc>& getNpcs() const;
    const std::vector<MapMonsterSpawn>& getMonsterSpawns() const;
    const std::vector<MapSafeZone>& getSafeZones() const;

    bool isSafe(float pX, float pZ) const;

    // ---- Height grid ----

    int getResolution() const;            // vertices per side
    float getGridSpacing() const;

    float getVertexX(int pX) const;
    float getVertexZ(int pZ) const;
    float getVertexHeight(int pX, int pZ) const;
    std::array<uint8_t, 3> getVertexColor(int pX, int pZ) const;

    // Follows the rendered triangles, so things stand exactly on what is
    // drawn. Outside the map the nearest edge is used.
    float getHeightAt(float pX, float pZ) const;

    // ---- Walkability ----

    bool isInside(float pX, float pZ) const;
    bool isBlocked(float pX, float pZ) const;

    // Inside the map, above water and not blocked.
    bool isWalkable(float pX, float pZ) const;

    // Checks the whole path, not just the destination, so a single long step
    // cannot jump through a wall. A move starting somewhere unwalkable is
    // always allowed - a character saved there must be able to walk out.
    bool isMoveAllowed(float pFromX, float pFromZ, float pToX, float pToZ) const;

    // Every point along the segment, pSampleStep apart, is walkable - no
    // escape clause, unlike isMoveAllowed.
    bool isPathWalkable(float pFromX, float pFromZ, float pToX, float pToZ, float pSampleStep = 1.0f) const;

    // ---- Walk grid (the block map's cells), for path finding ----

    int getCellResolution() const;
    float getCellSize() const;
    // The cell a coordinate falls in, on either axis (the map is square and
    // centred). Clamped to the map.
    int getCellIndex(float pCoordinate) const;
    float getCellCenterX(int pCellX) const;
    float getCellCenterZ(int pCellZ) const;

    // Walkable at the cell's centre. Precomputed at load.
    bool isCellWalkable(int pCellX, int pCellZ) const;
    bool isCellRoad(int pCellX, int pCellZ) const;

private:
    MapData() = default;

    uint32_t mId = 0;
    std::string mName;
    uint32_t mKingdom = 0;

    float mWorldSize = 0.0f;
    float mWaterLevel = 0.0f;

    MapSpawn mSpawn;
    std::vector<MapPortal> mPortals;
    std::vector<MapObject> mObjects;
    std::vector<MapNpc> mNpcs;
    std::vector<MapMonsterSpawn> mMonsterSpawns;
    std::vector<MapSafeZone> mSafeZones;

    int mResolution = 0;
    float mGridSpacing = 0.0f;
    std::vector<float> mHeights;          // mResolution * mResolution, row = z
    std::vector<uint8_t> mColors;         // same layout, 3 bytes each

    int mBlockResolution = 0;
    std::vector<uint8_t> mBlocked;        // 1 = blocked
    std::vector<uint8_t> mCellWalkable;   // 1 = walkable at the cell centre
    std::vector<uint8_t> mRoads;          // 1 = road; empty if the map has none
};

}

#endif
