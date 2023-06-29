#ifndef LAKOT_MOUSE_H
#define LAKOT_MOUSE_H

#include "platform.h"

#define LAKOT_MOUSE_MAXIMUM_BUTTON				8

#define LAKOT_MOUSE_DEFAULT_SENSIVITY			0.1f

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

	double mSensivity;

	bool mIsMouseInitialized;

	Button mButtons[LAKOT_MOUSE_MAXIMUM_BUTTON];

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

	double getSensivity();

	bool isButtonStateChanged(int pButtonId);

	bool isButtonPressed(int pButtonId);

	bool isButtonUp(int pButtonId);
	bool isButtonDown(int pButtonId);
};

#endif
