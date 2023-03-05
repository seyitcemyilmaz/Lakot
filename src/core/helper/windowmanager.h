#ifndef LAKOT_WINDOWMANAGER_H
#define LAKOT_WINDOWMANAGER_H

#include <glm/glm.hpp>

#include "platform.h"

#define LAKOT_DEFAULT_WINDOW_TITLE      "LAKOT"
#define LAKOT_DEFAULT_WINDOW_WIDTH      800
#define LAKOT_DEFAULT_WINDOW_HEIGHT     800
#define LAKOT_DEFAULT_WINDOW_BACKGROUND glm::vec3(0.3f, 0.2f, 0.5f)

class WindowManager {
private:
    static WindowManager* mInstance;

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

    WindowManager(const char* tWindowTitle, int tWindowWidth, int tWindowHeight, glm::vec3 tWindowBackgroundColor);
public:
    static WindowManager* getInstance();

    void initializateWindow();

    void updateWindow();

    ContextWindow* getWindow();

    void setWindowBackgroundColor(glm::vec3 tWindowBackgroundColor);
    glm::vec3 getWindowBackgroundColor();

    bool getIsWindowActive();

    int getWindowHeight();
    int getWindowWidth();

    static void frameBufferSizeCallback(ContextWindow* tWindow, int tWidth, int tHeight);
    static void windowCloseCallback(ContextWindow* tWindow);
};

#endif
