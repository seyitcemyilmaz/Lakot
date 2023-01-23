#include "lakot.h"

Lakot::Lakot() {
    mPlatform = nullptr;
    mWindowManager = nullptr;

    //mShaderString = "";

    mApplication = nullptr;
}

void Lakot::initializeLakot() {
    mPlatform = new Platform();
    mPlatform->getGraphicsAPI()->initializeGraphicsAPI();

    if (!mPlatform->getGraphicsAPI()->getIsGraphicsAPIInitialized()) {
        throw "Graphics API is not initialized.";
    }

    std::cout << "Lakot Graphics API: " << mPlatform->getGraphicsAPI()->getGraphicsAPIString() << std::endl;

    mWindowManager = new WindowManager("Lakot", 800, 800);
    mWindowManager->initializateWindow();
}

void Lakot::initalizeApplication() {
    mApplication = new Application();
    mApplication->initialization();
}

void Lakot::runApplication() {
    std::cout << "Application is running." << std::endl;
    #if LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGL
        while (mWindowManager->getIsWindowActive()) {
            mApplication->processInputs();
            mApplication->render();
            mWindowManager->updateWindow();
            glfwSwapBuffers(mWindowManager->getWindow());
            glfwPollEvents();
        }
    #elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGLES
        #error Not implemented.
    #elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_NONE
        #error Graphics API is not found.
    #else
        #error Undefined Graphics API.
    #endif
    std::cout << "Application is stopped." << std::endl;
}

/*
void Lakot::createShaderString() {
    OpenGLType tOpenGLType = mPlatform.getOpenGLType();
    OpenGLMajor tOpenGLMajor = mPlatform.getOpenGLMajor();
    OpenGLMinor tOpenGLMinor = mPlatform.getOpenGLMinor();

    if (tOpenGLType == OpenGLType::eOpenGLTypeCORE) {
        if (tOpenGLMajor == 3 && tOpenGLMinor == 3) {
            mShaderString = "#version 330";
            return;
        }
        throw "Undefined OpenGL major or minor version.";
    }

    if (tOpenGLType == OpenGLType::eOpenGLTypeES) {
        if (tOpenGLMajor == 3 && tOpenGLMinor == 2) {
            mShaderString = "#version 320 es";
            return;
        }
        throw "Undefined OpenGL major or minor version.";
    }

    throw "Undefined OpenGLType";
}

void Lakot::initApplication() {
    mApplication->initialization();
}

void Lakot::runApplication() {
    #if LAKOT_OPENGL_CONTEXT_PROVIDER == LAKOT_OPENGL_CONTEXT_PROVIDER_GLFW
    while (WindowManager::isWindowOpen()) {
        mApplication->loopBody();
    }
    #elif LAKOT_OPENGL_CONTEXT_PROVIDER == LAKOT_OPENGL_CONTEXT_PROVIDER_GLFM
        #error "GLFM is not implemented."
    #elif LAKOT_OPENGL_CONTEXT_PROVIDER == LAKOT_OPENGL_CONTEXT_PROVIDER_NONE
        #error "Context provider is not detected."
    #endif
}
*/

void Lakot::terminateApplication() {
    mPlatform->getGraphicsAPI()->terminateGraphicsAPI();
}

Lakot::~Lakot() {
    delete mWindowManager;
    delete mPlatform;
}
