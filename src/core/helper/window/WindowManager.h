#ifndef LAKOT_WINDOWMANAGER_H
#define LAKOT_WINDOWMANAGER_H

#include "Window.h"

class WindowManager
{
private:
    static WindowManager* mInstance;

    Window* mActiveWindow;

    WindowManager();
public:
    static WindowManager* getInstance();

    void initializateWindow();

    void updateWindow();

    Window* getWindow();

    void setWindowBackgroundColor(glm::vec3 tWindowBackgroundColor);
    glm::vec3 getWindowBackgroundColor();

    bool getIsWindowActive();
    bool getIsWindowFocused();

    int getWindowHeight();
    int getWindowWidth();

    void closeWindow();

    bool operator==(const WindowManager& other) = delete;
};

#endif
