#ifndef LAKOT_GLFWWINDOW_H
#define LAKOT_GLFWWINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "helper/window/Window.h"

class GLFWWindow final : public Window {
private:
    GLFWwindow* mWindowObject;

    void setCallbackFunctions();

    GLFWWindow(const char* pWindowTitle,
               int pWindowWidth,
               int pWindowHeight,
               const glm::vec3& pWindowBackgroundColor);

protected:
    friend class WindowFactory;

public:
    virtual ~GLFWWindow();

    void initialize() override;
    void createWindow() override;
    void updateWindow() override;
    void destroyWindow() override;

    void* getWindowInstance() override;

    static void frameBufferSizeCallback(GLFWwindow* pWindow, int pWidth, int pHeight);
    static void windowCloseCallback(GLFWwindow* pWindow);
    static void windowFocusCallback(GLFWwindow* pWindow, int pFocused);
};

#endif
