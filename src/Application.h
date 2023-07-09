#ifndef LAKOT_APPLICATION_H
#define LAKOT_APPLICATION_H

#include "application/graphics/model/Model.h"
#include "application/graphics/GUI.h"

#include "core/platform/Platform.h"

class Application {
private:
	Platform* mPlatform;

	GUI* mGUI;

	std::vector<Model*> mModels;

	void initializeShaders();
	void initializeCameras();
	void initializeModels();

public:
	Application();
	~Application();

	void initialization();
	void processInputs();
	void render();
	void run();
	void terminate();
};

#endif
