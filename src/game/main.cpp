#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <glad/glad.h>

#include "Engine.h"

SDL_AppResult SDL_AppInit(void** pEngine, int argc, char* argv[])
{
    lakot::Engine& tEngine = lakot::Engine::getInstance();
    *pEngine = &tEngine;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        return SDL_APP_FAILURE;
    }

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

    SDL_Window* tWindow = SDL_CreateWindow("Cross Platform Renderer", 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (!tWindow)
    {
        return SDL_APP_FAILURE;
    }

    SDL_GLContext tGLContext = SDL_GL_CreateContext(tWindow);

    if (!tGLContext)
    {
        return SDL_APP_FAILURE;
    }

    SDL_GL_MakeCurrent(tWindow, tGLContext);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        SDL_Log("Failed to initialize GLAD");
        return SDL_APP_FAILURE;
    }

    tEngine.setGLContext(tGLContext);
    tEngine.setWindow(tWindow);

    tEngine.initialize();

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
}
