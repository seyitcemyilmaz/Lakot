#ifndef LAKOT_APPLICATION_H
#define LAKOT_APPLICATION_H

#include "graphics/renderer.h"

class Application {
private:
    Renderer* mRenderer;
public:
    void initialization();
    void processInputs();
    void render();
};


#endif
