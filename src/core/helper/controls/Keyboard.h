#ifndef LAKOT_KEYBOARD_H
#define LAKOT_KEYBOARD_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct Key {
	bool isPressed;
	bool isStateChanged;
};

class Keyboard {
private:
	static Keyboard* mInstance;

	Key mKeys[GLFW_KEY_LAST];

	Keyboard();

public:
	static Keyboard* getInstance();

	static void keyCallback(GLFWwindow* pWindow, int pKeyId, int pScanCode, int pAction, int pMods);

	bool isKeyStateChanged(int pKeyId);

	bool isKeyPressed(int pKeyId);

	bool isKeyUp(int pKeyId);
	bool isKeyDown(int pKeyId);

	Keyboard(Keyboard&) = delete;
	void operator=(const Keyboard&) = delete;
};

#endif
