#ifndef LAKOT_MOUSE_H
#define LAKOT_MOUSE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define LAKOT_MOUSE_DEFAULT_SENSIVITY 0.5f

struct Button
{
    bool isPressed;
    bool isStateChanged;
};

class Mouse
{
private:
    static Mouse* mInstance;

    double mX;
    double mY;

    double mPreviousX;
    double mPreviousY;

    double mDX;
    double mDY;

    double mScrollDX;
    double mScrollDY;

    double mSensivity;

    bool mIsMouseInitialized;

    Button mButtons[GLFW_MOUSE_BUTTON_LAST];

    Mouse();

public:
    static Mouse* getInstance();

    static void cursorPositionCallBack(GLFWwindow* pWindow, double pX, double pY);
    static void cursorEnterCallback(GLFWwindow* pWindow, int pIsEntered);
    static void mouseButtonCallback(GLFWwindow* pWindow, int pButtonId, int pAction, int pMods);
    static void scrollCallback(GLFWwindow* pWindow, double pDX, double pDY);

    double getX();
    double getY();

    double getDX();
    double getDY();

    double getScrollDX();
    double getScrollDY();

    double getSensivity();

    bool isButtonStateChanged(int pButtonId);

    bool isButtonPressed(int pButtonId);

    bool isButtonUp(int pButtonId);
    bool isButtonDown(int pButtonId);
};

#endif
