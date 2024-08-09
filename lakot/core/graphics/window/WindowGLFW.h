#ifndef LAKOT_WINDOWGLFW_H
#define LAKOT_WINDOWGLFW_H

#include "Window.h"

class GLFWwindow;

namespace lakot {

class WindowGLFW final : public Window
{
public:
    virtual ~WindowGLFW() override;
    WindowGLFW();

    void initialize() override;
    void deinitialize() override;

    void update() override;
    void nextFrame() override;

private:
    GLFWwindow* mWindowObject;

    unsigned int mFPSFrameCounter;
    double mFPSPreviousTime;

    void setCallbackFunctions();

    static void frameBufferSizeCallback(GLFWwindow* pWindow, int pWidth, int pHeight);
    static void windowCloseCallback(GLFWwindow* pWindow);
    static void windowFocusCallback(GLFWwindow* pWindow, int pFocused);

};

}

#endif
