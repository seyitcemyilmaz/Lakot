#ifndef LAKOT_APPLICATION_H
#define LAKOT_APPLICATION_H

#include "application/resources/model.h"
#include "application/graphics/gui.h"

class Application {
private:
	GUI* mGUI;

	float mCurrentTime;
	float mPreviousTime;

	std::vector<Model*> mModels;

	void initializeShaders();
	void initializeCameras();
	void initializeModels();

	void processKeyboardInputs(float pDt);
	void processMouseInputs();
public:
	Application();
	~Application();

	void initialization();
	void processInputs();
	void render();
};

#endif
