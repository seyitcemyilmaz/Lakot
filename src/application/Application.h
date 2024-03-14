#ifndef LAKOT_APPLICATION_H
#define LAKOT_APPLICATION_H

#include <map>

#include "core/platform/Platform.h"
#include "core/helper/window/Window.h"
#include "application/graphics/gui/GUI.h"
#include "application/graphics/scene/Scene.h"

#include "application/graphics/PostProcess.h"

class Application
{
public:
    virtual ~Application();
    Application();

    void initialization();
    void processInputs();
    void render();
    void run();
    void terminate();

private:
    Platform* mPlatform;

    GUI* mGUI;

    Scene* mScene;

    PostProcess* mPostProcess;

    void initializeShaders();
    void initializeModels();
    void initializeParticles();

    void onResize(Window* pWindow, double pWidth, double pHeight);

    std::map<Window*, std::vector<Scene*>> mWindowSceneMap;
};

#endif
