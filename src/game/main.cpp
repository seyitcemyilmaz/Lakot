#define SDL_MAIN_USE_CALLBACKS 1

#include <string>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <glad/glad.h>

#include "Engine.h"
#include "FileManager.h"

#if defined(SDL_PLATFORM_WINDOWS)
#include <windows.h>

namespace
{
    HANDLE sShutdownFinished = nullptr;

    BOOL WINAPI onConsoleControl(DWORD pType)
    {
        SDL_Event tEvent{};
        tEvent.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&tEvent);

        if (pType != CTRL_C_EVENT && pType != CTRL_BREAK_EVENT)
        {
            WaitForSingleObject(sShutdownFinished, 4000);
        }

        return TRUE;
    }
}
#endif

SDL_AppResult SDL_AppInit(void** pEngine, int argc, char* argv[])
{
    lakot::Engine& tEngine = lakot::Engine::getInstance();
    *pEngine = &tEngine;

    SDL_SetHint(SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE, "0");

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        return SDL_APP_FAILURE;
    }

#if defined(SDL_PLATFORM_WINDOWS)
    sShutdownFinished = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    SetConsoleCtrlHandler(onConsoleControl, TRUE);
#endif

#if defined(SDL_PLATFORM_IOS)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#elif defined(SDL_PLATFORM_ANDROID)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* tWindow = nullptr;
    SDL_GLContext tGLContext = nullptr;

    for (int tSamples : { 4, 0 })
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, tSamples > 0 ? 1 : 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, tSamples);

        tWindow = SDL_CreateWindow("Lakot Online", 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

        if (!tWindow)
        {
            continue;
        }

        tGLContext = SDL_GL_CreateContext(tWindow);

        if (tGLContext)
        {
            break;
        }

        SDL_DestroyWindow(tWindow);
        tWindow = nullptr;
    }

    if (!tWindow || !tGLContext)
    {
        return SDL_APP_FAILURE;
    }

    // Below this, RmlUi's login card (and gameplay UI in general) has no
    // reasonable way to lay out without wrapping/overflowing - a login
    // dialog isn't a fluid webpage, so the fix is a sane floor on window
    // size rather than chasing arbitrarily small layouts.
    SDL_SetWindowMinimumSize(tWindow, 800, 600);

    SDL_GL_MakeCurrent(tWindow, tGLContext);

    // Vsync, adaptive where the driver supports it (a late frame tears
    // instead of waiting a whole extra refresh).
    if (!SDL_GL_SetSwapInterval(-1))
    {
        SDL_GL_SetSwapInterval(1);
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        SDL_Log("Failed to initialize GLAD");
        return SDL_APP_FAILURE;
    }

    tEngine.setGLContext(tGLContext);
    tEngine.setWindow(tWindow);

    // SDL_GetBasePath() is the directory containing the running executable,
    // resolved the same cross-platform way on every OS this ships to -
    // unlike anything keyed off LAKOT_EXTERNAL, which only exists at
    // build time on this dev machine.
    lakot::FileManager::setAssetPath(std::string(SDL_GetBasePath()) + "assets");

    if (!tEngine.initialize())
    {
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* pEngine, SDL_Event* pEvent)
{
    lakot::Engine* tEngine = static_cast<lakot::Engine*>(pEngine);
    return tEngine->eventHandler(pEvent);
}

SDL_AppResult SDL_AppIterate(void* pEngine)
{
    lakot::Engine* tEngine = static_cast<lakot::Engine*>(pEngine);

    tEngine->render();

    SDL_GL_SwapWindow(tEngine->getWindow());

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* pEngine, SDL_AppResult result)
{
    lakot::Engine* tEngine = static_cast<lakot::Engine*>(pEngine);

    tEngine->deinitialize();

    SDL_GL_DestroyContext(tEngine->getGLContext());
    SDL_DestroyWindow(tEngine->getWindow());

#if defined(SDL_PLATFORM_WINDOWS)
    if (sShutdownFinished)
    {
        SetEvent(sShutdownFinished);
    }
#endif
}
