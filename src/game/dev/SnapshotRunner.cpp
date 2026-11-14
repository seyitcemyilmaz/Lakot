#include "SnapshotRunner.h"

#if defined(LAKOT_DEV_TOOLS)

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <glad/glad.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace lakot;

namespace
{
    constexpr int kDefaultWidth = 2560;
    constexpr int kDefaultHeight = 1440;

    int readSizeVariable(int pIndex, int pFallback)
    {
        const char* tValue = SDL_getenv("LAKOT_SNAPSHOT_SIZE");

        if (!tValue)
        {
            return pFallback;
        }

        int tSize[2] = { 0, 0 };

        if (std::sscanf(tValue, "%dx%d", &tSize[0], &tSize[1]) != 2 || tSize[0] < 800 || tSize[1] < 600)
        {
            return pFallback;
        }

        return tSize[pIndex];
    }

    SDL_Keycode resolveKey(const std::string& pName)
    {
        if (pName == "esc")
        {
            return SDLK_ESCAPE;
        }

        if (pName == "enter")
        {
            return SDLK_RETURN;
        }

        SDL_Keycode tKey = SDL_GetKeyFromName(pName.c_str());

        if (tKey == SDLK_UNKNOWN && pName.size() == 1)
        {
            tKey = static_cast<SDL_Keycode>(pName[0]);
        }

        return tKey;
    }
}

bool SnapshotRunner::isRequested()
{
    const char* tScript = SDL_getenv("LAKOT_SNAPSHOT_SCRIPT");
    return tScript && *tScript;
}

int SnapshotRunner::getRequestedWidth()
{
    return readSizeVariable(0, kDefaultWidth);
}

int SnapshotRunner::getRequestedHeight()
{
    return readSizeVariable(1, kDefaultHeight);
}

bool SnapshotRunner::load(const std::string& pScriptPath, const std::string& pOutDir)
{
    std::ifstream tFile(pScriptPath);

    if (!tFile)
    {
        SDL_Log("Snapshot script could not be opened: %s", pScriptPath.c_str());
        return false;
    }

    mOutDir = pOutDir;
    std::filesystem::create_directories(mOutDir);

    std::string tLine;

    while (std::getline(tFile, tLine))
    {
        if (!parseLine(tLine))
        {
            SDL_Log("Snapshot script line ignored: %s", tLine.c_str());
        }
    }

    return true;
}

bool SnapshotRunner::parseLine(const std::string& pLine)
{
    std::istringstream tStream(pLine);
    std::string tCommand;

    if (!(tStream >> tCommand) || tCommand[0] == '#')
    {
        return true;
    }

    Step tStep;

    if (tCommand == "wait" || tCommand == "frames")
    {
        tStep.type = tCommand == "wait" ? StepType::eWaitMs : StepType::eWaitFrames;

        if (!(tStream >> tStep.number) || tStep.number < 0)
        {
            return false;
        }

        mSteps.push_back(tStep);
        return true;
    }

    if (tCommand == "key" || tCommand == "shot")
    {
        tStep.type = tCommand == "key" ? StepType::eKey : StepType::eShot;

        if (!(tStream >> tStep.text))
        {
            return false;
        }

        mSteps.push_back(tStep);
        return true;
    }

    if (tCommand == "text")
    {
        tStep.type = StepType::eText;
        std::getline(tStream >> std::ws, tStep.text);
        mSteps.push_back(tStep);
        return true;
    }

    if (tCommand == "click" || tCommand == "rclick" || tCommand == "dblclick" || tCommand == "move")
    {
        float tX = 0.0f;
        float tY = 0.0f;

        if (!(tStream >> tX >> tY))
        {
            return false;
        }

        if (tCommand == "move")
        {
            tStep.type = StepType::eMouseMove;
            tStep.x = tX;
            tStep.y = tY;
            mSteps.push_back(tStep);
            return true;
        }

        addClick(tX, tY, tCommand == "rclick" ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT, tCommand == "dblclick" ? 2 : 1);
        return true;
    }

    return false;
}

void SnapshotRunner::addClick(float pX, float pY, int pButton, int pCount)
{
    Step tMove;
    tMove.type = StepType::eMouseMove;
    tMove.x = pX;
    tMove.y = pY;
    mSteps.push_back(tMove);

    for (int tIndex = 0; tIndex < pCount; ++tIndex)
    {
        Step tDown = tMove;
        tDown.type = StepType::eMouseDown;
        tDown.button = pButton;
        tDown.number = tIndex + 1;
        mSteps.push_back(tDown);

        Step tUp = tDown;
        tUp.type = StepType::eMouseUp;
        mSteps.push_back(tUp);
    }
}

void SnapshotRunner::update(SDL_Window* pWindow)
{
    if (mIsFinished)
    {
        return;
    }

    if (mWaitFrames > 0)
    {
        --mWaitFrames;
        return;
    }

    if (SDL_GetTicks() < mWaitUntilMs)
    {
        return;
    }

    while (mNextStep < mSteps.size())
    {
        const Step& tStep = mSteps[mNextStep++];

        switch (tStep.type)
        {
            case StepType::eWaitMs:
                mWaitUntilMs = SDL_GetTicks() + static_cast<uint64_t>(tStep.number);
                return;
            case StepType::eWaitFrames:
                mWaitFrames = tStep.number;
                return;
            case StepType::eKey:
                pushKey(pWindow, tStep.text);
                return;
            case StepType::eText:
                pushText(pWindow, tStep.text);
                return;
            case StepType::eMouseMove:
            case StepType::eMouseDown:
            case StepType::eMouseUp:
                pushMouse(pWindow, tStep);
                return;
            case StepType::eShot:
                saveShot(pWindow, tStep.text);
                break;
        }
    }

    mIsFinished = true;

    SDL_Event tQuit{};
    tQuit.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&tQuit);
}

void SnapshotRunner::pushKey(SDL_Window* pWindow, const std::string& pName)
{
    SDL_Keycode tKey = resolveKey(pName);

    if (tKey == SDLK_UNKNOWN)
    {
        SDL_Log("Snapshot script key unknown: %s", pName.c_str());
        return;
    }

    SDL_Event tEvent{};
    tEvent.key.windowID = SDL_GetWindowID(pWindow);
    tEvent.key.key = tKey;
    SDL_Scancode tScancode = SDL_GetScancodeFromName(pName == "esc" ? "Escape" : pName == "enter" ? "Return" : pName.c_str());
    tEvent.key.scancode = tScancode != SDL_SCANCODE_UNKNOWN ? tScancode : SDL_GetScancodeFromKey(tKey, nullptr);
    tEvent.key.down = true;

    tEvent.type = SDL_EVENT_KEY_DOWN;
    SDL_PushEvent(&tEvent);

    tEvent.type = SDL_EVENT_KEY_UP;
    tEvent.key.down = false;
    SDL_PushEvent(&tEvent);
}

void SnapshotRunner::pushText(SDL_Window* pWindow, const std::string& pText)
{
    SDL_Event tEvent{};
    tEvent.type = SDL_EVENT_TEXT_INPUT;
    tEvent.text.windowID = SDL_GetWindowID(pWindow);
    tEvent.text.text = pText.c_str();
    SDL_PushEvent(&tEvent);
}

void SnapshotRunner::pushMouse(SDL_Window* pWindow, const Step& pStep)
{
    int tWidth = 1;
    int tHeight = 1;
    SDL_GetWindowSizeInPixels(pWindow, &tWidth, &tHeight);

    float tX = pStep.x * static_cast<float>(tWidth);
    float tY = pStep.y * static_cast<float>(tHeight);
    SDL_WindowID tWindowId = SDL_GetWindowID(pWindow);

    SDL_Event tEvent{};

    if (pStep.type == StepType::eMouseMove)
    {
        tEvent.type = SDL_EVENT_MOUSE_MOTION;
        tEvent.motion.windowID = tWindowId;
        tEvent.motion.x = tX;
        tEvent.motion.y = tY;
    }
    else
    {
        tEvent.type = pStep.type == StepType::eMouseDown ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
        tEvent.button.windowID = tWindowId;
        tEvent.button.button = static_cast<Uint8>(pStep.button);
        tEvent.button.down = pStep.type == StepType::eMouseDown;
        tEvent.button.clicks = static_cast<Uint8>(pStep.number);
        tEvent.button.x = tX;
        tEvent.button.y = tY;
    }

    SDL_PushEvent(&tEvent);
}

void SnapshotRunner::saveShot(SDL_Window* pWindow, const std::string& pName)
{
    int tWidth = 1;
    int tHeight = 1;
    SDL_GetWindowSizeInPixels(pWindow, &tWidth, &tHeight);

    std::vector<unsigned char> tPixels(static_cast<size_t>(tWidth) * static_cast<size_t>(tHeight) * 4);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, tWidth, tHeight, GL_RGBA, GL_UNSIGNED_BYTE, tPixels.data());

    for (size_t tIndex = 3; tIndex < tPixels.size(); tIndex += 4)
    {
        tPixels[tIndex] = 255;
    }

    std::string tPath = (std::filesystem::path(mOutDir) / (pName + ".png")).string();

    stbi_flip_vertically_on_write(1);
    stbi_write_png(tPath.c_str(), tWidth, tHeight, 4, tPixels.data(), tWidth * 4);

    SDL_Log("snapshot %s %dx%d", tPath.c_str(), tWidth, tHeight);
}

#endif
