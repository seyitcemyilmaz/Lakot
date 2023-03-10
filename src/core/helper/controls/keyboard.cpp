#include "keyboard.h"

Keyboard* Keyboard::mInstance = nullptr;

Keyboard* Keyboard::getInstance() {
    if (!mInstance) {
        mInstance = new Keyboard();
    }

    return mInstance;
}

Keyboard::Keyboard() {
    for (int i = 0 ; i < LAKOT_KEYBOARD_MAX_KEY; i++) {
        mKeys[i].isPressed = false;
        mKeys[i].isStateChanged = false;
    }
}

void Keyboard::keyCallback(ContextWindow* pWindow, int pKeyId, int pScanCode, int pAction, int pMods) {
#if LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGL
    Keyboard* tKeyboard = Keyboard::getInstance();

    if (!(pKeyId < LAKOT_KEYBOARD_MAX_KEY)) {
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
#elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGLES
    #error Not implemented.
#elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_NONE
    #error Graphics API is not found.
#else
    #error Undefined Graphics API.
#endif
}

bool Keyboard::isKeyStateChanged(int pKeyId) {
    if (!(pKeyId < LAKOT_KEYBOARD_MAX_KEY)) {
        throw "Key ID must be less than maximum key amount.";
    }

    bool tKeyState = mKeys[pKeyId].isStateChanged;
    mKeys[pKeyId].isStateChanged = false;
    return tKeyState;
}

bool Keyboard::isKeyPressed(int pKeyId) {
    if (!(pKeyId < LAKOT_KEYBOARD_MAX_KEY)) {
        throw "Key ID must be less than maximum key amount.";
    }

    return mKeys[pKeyId].isPressed;
}

bool Keyboard::isKeyUp(int pKeyId) {
    if (!(pKeyId < LAKOT_KEYBOARD_MAX_KEY)) {
        throw "Key ID must be less than maximum key amount.";
    }

    return !mKeys[pKeyId].isPressed && isKeyStateChanged(pKeyId);
}

bool Keyboard::isKeyDown(int pKeyId) {
    if (!(pKeyId < LAKOT_KEYBOARD_MAX_KEY)) {
        throw "Key ID must be less than maximum key amount.";
    }

    return mKeys[pKeyId].isPressed && isKeyStateChanged(pKeyId);
}
