#ifndef LAKOT_WINDOW_H
#define LAKOT_WINDOW_H

#include <functional>

#include <glm/glm.hpp>

class Window
{
public:
    virtual ~Window();

    virtual void initialize() = 0;
    virtual void createWindow() = 0;
    virtual void updateWindow() = 0;
    virtual void destroyWindow() = 0;

    virtual void* getWindowInstance() = 0;

    void setWindowWidth(int pWindowWidth);
    void setWindowHeight(int pWindowHeight);

    void setIsWindowActive(bool pIsWindowActive);
    void setIsWindowFocused(bool pIsWindowFocused);

    void setWindowBackgroundColor(const glm::vec3& pWindowBackgroundColor);

    int getWindowWidth();
    int getWindowHeight();

    bool getIsWindowActive();
    bool getIsWindowFocused();

    const glm::vec3& getWindowBackgroundColor();

    void setOnResize(std::function<void (Window*, double, double)> pOnResizeFunction);

protected:
    const char* mWindowTitle;

    int mWindowWidth;
    int mWindowHeight;

    bool mIsWindowActive;
    bool mIsWindowFocused;

    glm::vec3 mWindowBackgroundColor;

    std::function<void (Window*, double, double)> mOnResizeFunction;

    Window(const char* pWindowTitle, int pWindowWidth, int pWindowHeight, const glm::vec3& pWindowBackgroundColor);
};

#endif
