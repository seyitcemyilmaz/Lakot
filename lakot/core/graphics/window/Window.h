#ifndef LAKOT_WINDOW_H
#define LAKOT_WINDOW_H

#include <lakot/abstract/graphics/window/AWindow.h>

namespace lakot {

class Window : public AWindow
{
public:
    virtual ~Window() override;
    Window();

    static Window* getInstance();

    virtual void update() override = 0;

protected:
    static Window* mInstance;
};

}

#endif
