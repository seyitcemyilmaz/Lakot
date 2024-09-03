#include "WindowEGL.h"

#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>

#include <spdlog/spdlog.h>

#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>

using namespace lakot;

android_app* WindowEGL::mAndroidApp = nullptr;

extern int main();

extern "C" {

#include <game-activity/native_app_glue/android_native_app_glue.c>

void handleCommand(android_app* pAndroidApp, int32_t pCommand)
{
    switch (pCommand) {
        case APP_CMD_INIT_WINDOW:
            // A new window is created, associate a renderer with it. You may replace this with a
            // "game" class if that suits your needs. Remember to change all instances of userData
            // if you change the class here as a reinterpret_cast is dangerous this in the
            // android_main function and the APP_CMD_TERM_WINDOW handler case.
            pAndroidApp->userData = Window::getInstance();
            ((WindowEGL*) Window::getInstance())->createEGLContext();
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being destroyed. Use this to clean up your userData to avoid leaking
            // resources.
            //
            // We have to check if userData is assigned just in case this comes in really quickly
            if (pAndroidApp->userData) {
                //
//                auto *pRenderer = reinterpret_cast<Renderer *>(pAndroidApp->userData);
//                pAndroidApp->userData = nullptr;
//                delete pRenderer;
            }
            break;
        default:
            break;
    }
}

bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent)
{
    auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
    return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
            sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
}

void android_main(struct android_app* pApp)
{
    WindowEGL::setAndroidApp(pApp);

    main();
}

}

WindowEGL::~WindowEGL()
{

}

WindowEGL::WindowEGL()
    : Window()
    , mDisplay(EGL_NO_DISPLAY)
    , mSurface(EGL_NO_SURFACE)
    , mContext(EGL_NO_CONTEXT)
{
    mTitle = "EGL Window";
    mWidth = -1;
    mHeight = -1;
}

void WindowEGL::initialize()
{
    spdlog::info("EGL Window is initializing.");

    mInstance = this;

    mIsInitialized = true;

    mAndroidApp->onAppCmd = handleCommand;
    android_app_set_motion_event_filter(mAndroidApp, motion_event_filter_func);

    spdlog::info("EGL Window is initialized.");
}

void WindowEGL::deinitialize()
{
    spdlog::info("EGL Window is deinitializing.");

    spdlog::info("EGL Window is deinitialized.");
}

void WindowEGL::createEGLContext()
{
    constexpr EGLint attribs[] =
    {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_NONE
    };

    // The default display is probably what you want on Android
    auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    // figure out how many configs there are
    EGLint numConfigs;
    eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);

    // get the list of configurations
    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);

    // Find a config we like.
    // Could likely just grab the first if we don't care about anything else in the config.
    // Otherwise hook in your own heuristic
    auto config = *std::find_if(
            supportedConfigs.get(),
            supportedConfigs.get() + numConfigs,
            [&display](const EGLConfig &config) {
                EGLint red, green, blue, depth;
                if (eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red) &&
                    eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green) &&
                    eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue) &&
                    eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth))
                {
                    return red == 8 && green == 8 && blue == 8 && depth == 24;
                }
                return false;
            });

    // create the proper window surface
    EGLint format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    EGLSurface surface = eglCreateWindowSurface(display, config, mAndroidApp->window, nullptr);
    // Create a GLES 3 context
    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, nullptr, contextAttribs);

    // get some window metrics
    auto madeCurrent = eglMakeCurrent(display, surface, surface, context);
    assert(madeCurrent);

    mDisplay = display;
    mSurface = surface;
    mContext = context;
}

void WindowEGL::update()
{
    bool done = false;
    while (!done)
    {
        // 0 is non-blocking.
        int timeout = 0;
        int events;
        android_poll_source *pSource;
        int result = ALooper_pollOnce(timeout, nullptr, &events,
                                      reinterpret_cast<void**>(&pSource));
        switch (result) {
            case ALOOPER_POLL_TIMEOUT:
                [[clang::fallthrough]];
            case ALOOPER_POLL_WAKE:// No events occurred before the timeout or explicit wake. Stop checking for events.
                done = true;
                break;
            case ALOOPER_EVENT_ERROR:
                spdlog::error("ALooper_pollOnce returned an error");
                break;
            case ALOOPER_POLL_CALLBACK:break;default:if (pSource)
            {
                pSource->process(mAndroidApp, pSource);
            }
        }
    }

    if (mAndroidApp->userData)
    {
        // We know that our user data is a Renderer, so reinterpret cast it. If you change your
        // user data remember to change it here
        auto *pRenderer = mAndroidApp->userData;

        // Process game input
//            pRenderer->handleInput();

        // Render a frame
//            pRenderer->render();

        glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        spdlog::info("render");
    }

    if (mAndroidApp->destroyRequested)
    {
        mIsActive = false;
    }
}

void WindowEGL::nextFrame()
{
    if (mAndroidApp->userData)
    {
        eglSwapBuffers(mDisplay, mSurface);
    }
}

void WindowEGL::setAndroidApp(android_app *pAndroidApp) {
    mAndroidApp = pAndroidApp;
}
