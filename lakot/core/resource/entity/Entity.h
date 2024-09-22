#ifndef LAKOT_ENTITY_H
#define LAKOT_ENTITY_H

#include <lakot/abstract/resource/entity/AEntity.h>

namespace lakot {

class Entity : public AEntity
{
public:
    virtual ~Entity() override;
    Entity();
};

}

#endif
