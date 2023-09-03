#ifndef LAKOT_WINDOWMANAGER_H
#define LAKOT_WINDOWMANAGER_H

#include "Window.h"

class WindowManager {
private:
	static WindowManager* mInstance;

	Window* mActiveWindow = nullptr;

	WindowManager();

public:
	static WindowManager* getInstance();

	void initializateWindow();

	void updateWindow();

	void* getWindow();

	void setWindowBackgroundColor(glm::vec3 tWindowBackgroundColor);
	glm::vec3 getWindowBackgroundColor();

	bool getIsWindowActive();

	int getWindowHeight();
	int getWindowWidth();

	void closeWindow();

	bool operator==(const WindowManager& other) = delete;
};

#endif
