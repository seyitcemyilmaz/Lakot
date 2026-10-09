#ifndef LAKOT_WORLD_ENTITY_H
#define LAKOT_WORLD_ENTITY_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "../../graphics/model/Animator.h"
#include "../../graphics/model/SkinnedModel.h"
#include "AnimationRoles.h"
#include "WorldViewConstants.h"

#include "../../network/GameTypes.h"

namespace lakot
{

enum class EntityType
{
    ePlayer,
    eMonster,
    eNpc,
    eGroundItem
};

struct WorldEntity
{
    uint64_t id{0};
    EntityType type{EntityType::ePlayer};
    std::string name;

    glm::vec3 previousPosition{0.0f};
    glm::vec3 targetPosition{0.0f};
    double interpolationElapsed{0.0};
    double interpolationDuration{1.0};

    float facing{0.0f};
    double movingTimer{0.0};
    glm::vec3 lastAnimatedPosition{0.0f};

    std::shared_ptr<SkinnedModel> model;
    std::unique_ptr<Animator> animator;
    AnimationRoleClips clips;

    uint32_t kingdom{0};
    uint32_t level{0};
    glm::vec3 tint{1.0f};
    float scale{1.0f};
    uint32_t healthPercent{100};
    bool isDead{false};

    std::vector<EquippedVisual> equipment;

    uint32_t itemTemplateId{0};
    uint32_t itemCount{0};
    uint64_t gold{0};

    std::string speechText;
    double speechRemainingSeconds{0.0};

    glm::vec3 getHeadTopOffset() const
    {
        return glm::vec3(0.0f, WorldView::kPlayerBoxHalfExtents.y * (2.0f * scale - 1.0f), 0.0f);
    }
};

}

#endif
