#ifndef LAKOT_KEYBOARD_H
#define LAKOT_KEYBOARD_H

#include "platform.h"

#define LAKOT_KEYBOARD_MAX_KEY  512

struct Key {
    bool isPressed;
    bool isStateChanged;
};

class Keyboard {
private:
    static Keyboard* mInstance;

    Key mKeys[LAKOT_KEYBOARD_MAX_KEY];

    Keyboard();
public:
    static Keyboard* getInstance();

    static void keyCallback(ContextWindow* pWindow, int pKeyId, int pScanCode, int pAction, int pMods);

    bool isKeyStateChanged(int pKeyId);

    bool isKeyPressed(int pKeyId);

    bool isKeyUp(int pKeyId);
    bool isKeyDown(int pKeyId);
};

#endif
