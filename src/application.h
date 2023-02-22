#ifndef LAKOT_APPLICATION_H
#define LAKOT_APPLICATION_H

#include "application/graphics/shadermanager.h"

#include "application/graphics/render/renderer.h"

class Application {
private:
    Renderer* mRenderer;
    ShaderManager* mShaderManager;

    void initializeShaders();
public:
    Application();
    ~Application();

    void initialization();
    void processInputs();
    void render();
};


#endif
