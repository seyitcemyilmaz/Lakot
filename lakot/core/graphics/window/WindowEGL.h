#ifndef LAKOT_WINDOWEGL_H
#define LAKOT_WINDOWEGL_H

#include "Window.h"

#include <EGL/egl.h>

extern "C" {
#include <game-activity/native_app_glue/android_native_app_glue.h>
}

namespace lakot {

class WindowEGL final : public Window
{
public:
    virtual ~WindowEGL() override;
    WindowEGL();

    void initialize() override;
    void deinitialize() override;

    void createEGLContext();

    void update() override;
    void nextFrame() override;

    static void setAndroidApp(android_app* pAndroidApp);
    static android_app* getAndroidApp();

    const EGLDisplay& getDisplay() const;
    const EGLSurface& getSurface() const;

private:
    EGLDisplay mDisplay;
    EGLSurface mSurface;
    EGLContext mContext;

    static android_app* mAndroidApp;

    unsigned int mFPSFrameCounter;
    double mFPSPreviousTime;
};

}

#endif
