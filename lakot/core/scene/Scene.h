#ifndef LAKOT_SCENE_H
#define LAKOT_SCENE_H

#include <lakot/abstract/scene/AScene.h>

#include "../graphics/geometry/Box.h"

namespace lakot {

class Scene : public AScene
{
public:
    virtual ~Scene() override;
    Scene();

    void initialize() override;
    void deinitialize() override;

    void update() override;

private:
    Box* mBox;
};

}

#endif
