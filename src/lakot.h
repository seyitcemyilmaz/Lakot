#ifndef LAKOT_LAKOT_H
#define LAKOT_LAKOT_H

#include "core/platform.h"
#include "core/helper/windowmanager.h"

#include "application.h"

class Lakot {
private:
    Platform* mPlatform;

    Application* mApplication;

    void initializeCallbackFunctions();
public:
    Lakot();
    ~Lakot();

    void initializeLakot();

    void initalizeApplication();

    void runApplication();

    void terminateApplication();
};

#endif
