#ifndef LAKOT_WINDOWMANAGER_H
#define LAKOT_WINDOWMANAGER_H

#include "platform.h"

#include <glm/glm.hpp>

class WindowManager {
private:
    ContextWindow* mWindow;

    bool mIsWindowActive;

    const char* mWindowTitle;

    int mWindowWidth;
    int mWindowHeight;

    glm::vec3 mWindowBackgroundColor;

    void setWindowWidth(int tWindowWidth);
    void setWindowHeight(int tWindowHeight);

    void setIsWindowActive(bool tIsWindowActive);

    void createWindow();
public:
    WindowManager(const char* tWindowTitle, int tWindowWidth, int tWindowHeight, glm::vec3 tWindowBackgroundColor = glm::vec3(1.0f, 0.3f, 0.3f));
    ~WindowManager();
    void initializateWindow();

    void updateWindow();

    ContextWindow* getWindow();

    void setWindowBackgroundColor(glm::vec3 tWindowBackgroundColor);
    glm::vec3 getWindowBackgroundColor();

    bool getIsWindowActive();

    static void frameBufferSizeCallback(ContextWindow* tWindow, int tWidth, int tHeight);
    static void windowCloseCallback(ContextWindow* tWindow);
};

#endif
