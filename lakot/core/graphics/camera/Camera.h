#ifndef LAKOT_CAMERA_H
#define LAKOT_CAMERA_H

#include <lakot/abstract/graphics/camera/ACamera.h>

namespace lakot {

class Camera : public ACamera
{
public:
    virtual ~Camera() override;
    Camera();
};

}

#endif
