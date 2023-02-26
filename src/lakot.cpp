#include "lakot.h"

#include "helper/controls/mouse.h"

Lakot::Lakot() {
    mPlatform = nullptr;
    mWindowManager = nullptr;

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

    initializeCallbackFunctions();
}

void Lakot::initializeCallbackFunctions() {
#if LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGL
    glfwSetWindowUserPointer(mWindowManager->getWindow(), mWindowManager);
    glfwSetFramebufferSizeCallback(mWindowManager->getWindow(), WindowManager::frameBufferSizeCallback);
    glfwSetWindowCloseCallback(mWindowManager->getWindow(), WindowManager::windowCloseCallback);
    //glfwSetKeyCallback(mWindowManager->getWindow(), );
    glfwSetMouseButtonCallback(mWindowManager->getWindow(), Mouse::mouseButtonCallback);
    glfwSetScrollCallback(mWindowManager->getWindow(), Mouse::scrollCallback);
    glfwSetCursorPosCallback(mWindowManager->getWindow(), Mouse::cursorPositionCallBack);
#elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGLES
    #error Not implemented.
#elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_NONE
    #error Graphics API is not found.
#else
    #error Undefined Graphics API.
#endif
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

void Lakot::terminateApplication() {
    mPlatform->getGraphicsAPI()->terminateGraphicsAPI();
}

Lakot::~Lakot() {
    delete mWindowManager;
    delete mPlatform;
}
