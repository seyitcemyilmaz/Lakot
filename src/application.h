#ifndef LAKOT_APPLICATION_H
#define LAKOT_APPLICATION_H

#include "application/graphics/render/renderer.h"
#include "application/graphics/model.h"

class Application {
private:
    Renderer* mRenderer;

    std::vector<Model*> mModels;

    void initializeShaders();
    void initializeCameras();
public:
    Application();
    ~Application();

    void initialization();
    void processInputs();
    void render();
};


#endif
