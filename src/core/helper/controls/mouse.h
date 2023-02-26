#ifndef LAKOT_MOUSE_H
#define LAKOT_MOUSE_H

#include "platform.h"

#define LAKOT_MAXIMUM_MOUSE_BUTTON 8

struct Button {
    bool isPressed;
    bool isStateChanged;
};

class Mouse {
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

    bool mIsMouseInitialized;

    Button mButtons[LAKOT_MAXIMUM_MOUSE_BUTTON];

    Mouse();
public:
    static Mouse* getInstance();

    static void cursorPositionCallBack(ContextWindow* pWindow, double pX, double pY);
    static void mouseButtonCallback(ContextWindow* pWindow, int pButtonId, int pAction, int pMods);
    static void scrollCallback(ContextWindow* pWindow, double pDX, double pDY);

    double getX();
    double getY();

    double getDX();
    double getDY();

    double getScrollDX();
    double getScrollDY();

    bool isButtonStateChanged(int pButtonId);

    bool isButtonPressed(int pButtonId);

    bool isButtonUp(int pButtonId);
    bool isButtonDown(int pButtonId);
};

#endif
