#ifndef LAKOT_WORLD_PICKER_H
#define LAKOT_WORLD_PICKER_H

#include <cstdint>
#include <optional>
#include <string>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "MapData.h"

#include "WorldEntity.h"

namespace lakot
{

class Camera;
class WorldEntities;

class WorldPicker
{
public:
    struct Target
    {
        EntityType type;
        uint64_t id;
        std::string name;
    };

    WorldPicker(SDL_Window* pWindow, const Camera& pCamera, const WorldEntities& pEntities);

    std::optional<glm::vec3> pickGround(const MapData& pMap, const glm::vec2& pWindowPixel) const;

    std::optional<Target> pickTarget(const glm::vec2& pWindowPixel) const;

    static glm::vec3 getObjectCenter(const MapData& pMap, const MapObject& pObject);

private:
    struct Ray
    {
        glm::vec3 origin;
        glm::vec3 direction;
    };

    SDL_Window* mWindow;
    const Camera& mCamera;
    const WorldEntities& mEntities;

    std::optional<Ray> makeRay(const glm::vec2& pWindowPixel) const;
};

}

#endif
