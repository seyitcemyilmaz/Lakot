#ifndef LAKOT_WINDOWEGL_H
#define LAKOT_WINDOWEGL_H

#include "Window.h"

namespace lakot {

class WindowEGL final : public Window
{
public:
    virtual ~WindowEGL() override;
    WindowEGL();

    void initialize() override;
    void deinitialize() override;

    void update() override;
    void nextFrame() override;
};

}

#endif
