#include "Keyboard.h"

Keyboard* Keyboard::mInstance = nullptr;

Keyboard* Keyboard::getInstance() {
	if (!mInstance) {
		mInstance = new Keyboard();
	}

	return mInstance;
}

Keyboard::Keyboard() {
	for (int i = 0 ; i < GLFW_KEY_LAST; i++) {
		mKeys[i].isPressed = false;
		mKeys[i].isStateChanged = false;
	}
}

void Keyboard::keyCallback(GLFWwindow* pWindow, int pKeyId, int pScanCode, int pAction, int pMods) {
	Keyboard* tKeyboard = Keyboard::getInstance();

	if (!(pKeyId < GLFW_KEY_LAST)) {
		throw "Key ID must be less than maximum key amount.";
	}

	if (pAction != GLFW_RELEASE) {
		if (!tKeyboard->mKeys[pKeyId].isPressed) {
			tKeyboard->mKeys[pKeyId].isPressed = true;
		}
	}
	else {
		tKeyboard->mKeys[pKeyId].isPressed = false;
	}
	tKeyboard->mKeys[pKeyId].isStateChanged = pAction != GLFW_REPEAT;
}

bool Keyboard::isKeyStateChanged(int pKeyId) {
	if (!(pKeyId < GLFW_KEY_LAST)) {
		throw "Key ID must be less than maximum key amount.";
	}

	bool tKeyState = mKeys[pKeyId].isStateChanged;
	mKeys[pKeyId].isStateChanged = false;
	return tKeyState;
}

bool Keyboard::isKeyPressed(int pKeyId) {
	if (!(pKeyId < GLFW_KEY_LAST)) {
		throw "Key ID must be less than maximum key amount.";
	}

	return mKeys[pKeyId].isPressed;
}

bool Keyboard::isKeyUp(int pKeyId) {
	if (!(pKeyId < GLFW_KEY_LAST)) {
		throw "Key ID must be less than maximum key amount.";
	}

	return !mKeys[pKeyId].isPressed && isKeyStateChanged(pKeyId);
}

bool Keyboard::isKeyDown(int pKeyId) {
	if (!(pKeyId < GLFW_KEY_LAST)) {
		throw "Key ID must be less than maximum key amount.";
	}

	return mKeys[pKeyId].isPressed && isKeyStateChanged(pKeyId);
}
