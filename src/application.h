#ifndef LAKOT_APPLICATION_H
#define LAKOT_APPLICATION_H

#include "application/resources/model.h"
#include "application/graphics/gui.h"

class Application {
private:
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
};

#endif
