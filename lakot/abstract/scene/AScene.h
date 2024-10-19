#ifndef LAKOT_ASCENE_H
#define LAKOT_ASCENE_H

#include <lakot/utilities/Object.h>
#include "../graphics/camera/ACamera.h"

namespace lakot {

class AScene : public Object
{
public:
    virtual ~AScene() override;
    AScene();

    virtual void update() = 0;

    ACamera* getActiveCamera() const;

protected:
    ACamera* mActiveCamera;
};

}

#endif
