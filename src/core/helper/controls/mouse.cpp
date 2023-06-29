#include "mouse.h"

Mouse* Mouse::mInstance = nullptr;

Mouse* Mouse::getInstance() {
	if (!mInstance) {
		mInstance = new Mouse();
	}

	return mInstance;
}

Mouse::Mouse() {
	mX = 0;
	mY = 0;

	mPreviousX = 0;
	mPreviousY = 0;

	mDX = 0;
	mDY = 0;

	mScrollDX = 0;
	mScrollDY = 0;

	mSensivity = LAKOT_MOUSE_DEFAULT_SENSIVITY;

	mIsMouseInitialized = false;

	for (int i = 0 ; i < LAKOT_MOUSE_MAXIMUM_BUTTON; i++) {
		mButtons[i].isPressed = false;
		mButtons[i].isStateChanged = false;
	}
}

void Mouse::cursorPositionCallBack(ContextWindow* pWindow, double pX, double pY) {
	Mouse* tMouse = Mouse::getInstance();

	tMouse->mX = pX;
	tMouse->mY = pY;

	if (!tMouse->mIsMouseInitialized) {
		tMouse->mPreviousX = tMouse->mX;
		tMouse->mPreviousY = tMouse->mY;

		tMouse->mIsMouseInitialized = true;
	}

	tMouse->mDX = tMouse->mX - tMouse->mPreviousX;
	tMouse->mDY = tMouse->mPreviousY - tMouse->mY;

	tMouse->mPreviousX = tMouse->mX;
	tMouse->mPreviousY = tMouse->mY;
}

void Mouse::mouseButtonCallback(ContextWindow* pWindow, int pButtonId, int pAction, int pMods) {
	Mouse* tMouse = Mouse::getInstance();

#if LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGL
	if (!(pButtonId < LAKOT_MOUSE_MAXIMUM_BUTTON)) {
		throw "Button ID must be less than maximum mouse button amount.";
	}

	if (pAction != GLFW_RELEASE) {
		if (!tMouse->mButtons[pButtonId].isPressed) {
			tMouse->mButtons[pButtonId].isPressed = true;
		}
	}
	else {
		tMouse->mButtons[pButtonId].isPressed = false;
	}

	tMouse->mButtons[pButtonId].isStateChanged = (pAction != GLFW_REPEAT);
#elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGLES
#error Not implemented.
#elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_NONE
#error Graphics API is not found.
#else
#error Undefined Graphics API.
#endif
}

void Mouse::scrollCallback(ContextWindow* pWindow, double pDX, double pDY) {
	Mouse* tMouse = Mouse::getInstance();

	tMouse->mScrollDX = pDX;
	tMouse->mScrollDY = pDY;
}

double Mouse::getX() {
	return mX;
}

double Mouse::getY() {
	return mY;
}

double Mouse::getDX() {
	double tDX = mDX;
	mDX = 0;
	return tDX;
}

double Mouse::getDY() {
	double tDY = mDY;
	mDY = 0;
	return tDY;
}

double Mouse::getScrollDX() {
	double tScrollDX = mScrollDX;
	mScrollDX = 0;
	return tScrollDX;
}

double Mouse::getScrollDY() {
	double tScrollDY = mScrollDY;
	mScrollDY = 0;
	return tScrollDY;
}

double Mouse::getSensivity() {
	return mSensivity;
}

bool Mouse::isButtonStateChanged(int pButtonId) {
	if (!(pButtonId < LAKOT_MOUSE_MAXIMUM_BUTTON)) {
		throw "Button ID must be less than maximum mouse button amount.";
	}

	bool tButtonState = mButtons[pButtonId].isStateChanged;
	mButtons[pButtonId].isStateChanged = false;
	return tButtonState;
}

bool Mouse::isButtonPressed(int pButtonId) {
	if (!(pButtonId < LAKOT_MOUSE_MAXIMUM_BUTTON)) {
		throw "Button ID must be less than maximum mouse button amount.";
	}

	return mButtons[pButtonId].isPressed;
}

bool Mouse::isButtonUp(int pButtonId) {
	if (!(pButtonId < LAKOT_MOUSE_MAXIMUM_BUTTON)) {
		throw "Button ID must be less than maximum mouse button amount.";
	}

	return !mButtons[pButtonId].isPressed && isButtonStateChanged(pButtonId);
}

bool Mouse::isButtonDown(int pButtonId) {
	if (!(pButtonId < LAKOT_MOUSE_MAXIMUM_BUTTON)) {
		throw "Button ID must be less than maximum mouse button amount.";
	}

	return mButtons[pButtonId].isPressed && isButtonStateChanged(pButtonId);
}
