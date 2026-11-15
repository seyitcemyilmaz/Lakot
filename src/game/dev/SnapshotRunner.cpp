#include "SnapshotRunner.h"

#if defined(LAKOT_DEV_TOOLS)

#include <algorithm>
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
    const char* tLive = SDL_getenv("LAKOT_SNAPSHOT_LIVE");
    return (tScript && *tScript) || (tLive && *tLive);
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
    mOutDir = pOutDir;
    std::filesystem::create_directories(mOutDir);

    const char* tLive = SDL_getenv("LAKOT_SNAPSHOT_LIVE");

    if (tLive && *tLive)
    {
        mLiveDir = tLive;
        std::filesystem::create_directories(mLiveDir);
        return true;
    }

    return loadScript(pScriptPath);
}

bool SnapshotRunner::loadScript(const std::string& pPath)
{
    std::ifstream tFile(pPath);

    if (!tFile)
    {
        SDL_Log("Snapshot script could not be opened: %s", pPath.c_str());
        return false;
    }

    std::string tLine;

    while (std::getline(tFile, tLine))
    {
        if (!parseLine(tLine))
        {
            SDL_Log("Snapshot script line ignored: %s", tLine.c_str());
            ++mIgnoredLines;
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

    if (tCommand == "quit")
    {
        tStep.type = StepType::eQuit;
        mSteps.push_back(tStep);
        return true;
    }

    if (tCommand == "keyhold")
    {
        std::string tName;
        int tMs = 0;

        if (!(tStream >> tName >> tMs) || tMs < 0 || tMs > 60000 || resolveKey(tName) == SDLK_UNKNOWN)
        {
            return false;
        }

        addKeyHold(tName, tMs);
        return true;
    }

    if (tCommand == "drag")
    {
        return parseDrag(tStream);
    }

    if (tCommand == "key" || tCommand == "keydown" || tCommand == "keyup" || tCommand == "shot")
    {
        tStep.type = tCommand == "key" ? StepType::eKey : tCommand == "keydown" ? StepType::eKeyDown : tCommand == "keyup" ? StepType::eKeyUp : StepType::eShot;

        if (!(tStream >> tStep.text) || (tStep.type != StepType::eShot && resolveKey(tStep.text) == SDLK_UNKNOWN))
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

    if (tCommand == "orbit" || tCommand == "wheel")
    {
        float tX = 0.0f;
        float tY = 0.0f;

        if (!(tStream >> tX) || (tCommand == "orbit" && !(tStream >> tY)))
        {
            return false;
        }

        if (tCommand == "wheel")
        {
            tStep.type = StepType::eWheel;
            tStep.y = tX;
            mSteps.push_back(tStep);
            return true;
        }

        addOrbit(tX, tY);
        return true;
    }

    return false;
}

bool SnapshotRunner::parseDrag(std::istringstream& pStream)
{
    float tX1 = 0.0f;
    float tY1 = 0.0f;
    float tX2 = 0.0f;
    float tY2 = 0.0f;
    int tMs = 0;

    if (!(pStream >> tX1 >> tY1 >> tX2 >> tY2 >> tMs) || tMs < 0 || tMs > 60000)
    {
        return false;
    }

    for (float tValue : { tX1, tY1, tX2, tY2 })
    {
        if (tValue < 0.0f || tValue > 1.0f)
        {
            return false;
        }
    }

    std::string tButtonName;
    int tButton = SDL_BUTTON_RIGHT;

    if (pStream >> tButtonName)
    {
        if (tButtonName != "left" && tButtonName != "right")
        {
            return false;
        }

        tButton = tButtonName == "left" ? SDL_BUTTON_LEFT : SDL_BUTTON_RIGHT;
    }

    addDrag(tX1, tY1, tX2, tY2, tMs, tButton);
    return true;
}

void SnapshotRunner::addKeyHold(const std::string& pName, int pMs)
{
    Step tDown;
    tDown.type = StepType::eKeyDown;
    tDown.text = pName;
    mSteps.push_back(tDown);

    Step tWait;
    tWait.type = StepType::eWaitMs;
    tWait.number = pMs;
    mSteps.push_back(tWait);

    Step tUp = tDown;
    tUp.type = StepType::eKeyUp;
    mSteps.push_back(tUp);
}

void SnapshotRunner::addDrag(float pX1, float pY1, float pX2, float pY2, int pMs, int pButton)
{
    Step tMove;
    tMove.type = StepType::eMouseMove;
    tMove.x = pX1;
    tMove.y = pY1;
    mSteps.push_back(tMove);

    Step tDown = tMove;
    tDown.type = StepType::eMouseDown;
    tDown.button = pButton;
    tDown.number = 1;
    mSteps.push_back(tDown);

    const int tSegments = std::max(1, pMs / 50);

    for (int tIndex = 1; tIndex <= tSegments; ++tIndex)
    {
        const float tFraction = static_cast<float>(tIndex) / static_cast<float>(tSegments);

        Step tSegment = tMove;
        tSegment.x = pX1 + (pX2 - pX1) * tFraction;
        tSegment.y = pY1 + (pY2 - pY1) * tFraction;
        mSteps.push_back(tSegment);

        Step tWait;
        tWait.type = StepType::eWaitMs;
        tWait.number = pMs / tSegments;
        mSteps.push_back(tWait);
    }

    Step tUp = tDown;
    tUp.type = StepType::eMouseUp;
    tUp.x = pX2;
    tUp.y = pY2;
    mSteps.push_back(tUp);
}

void SnapshotRunner::addOrbit(float pDeltaX, float pDeltaY)
{
    Step tMove;
    tMove.type = StepType::eMouseMove;
    tMove.x = 0.5f;
    tMove.y = 0.5f;
    mSteps.push_back(tMove);

    Step tDrag = tMove;
    tDrag.type = StepType::eMouseDrag;
    tDrag.x = pDeltaX;
    tDrag.y = pDeltaY;
    mSteps.push_back(tDrag);
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

    if (!mLiveDir.empty() && mNextStep >= mSteps.size() && mWaitFrames <= 0 && SDL_GetTicks() >= mWaitUntilMs)
    {
        releaseHeldKeys(pWindow);
        updateLive();

        if (mNextStep >= mSteps.size())
        {
            return;
        }
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
                pushKey(pWindow, tStep.text, KeyPhase::eTap);
                return;
            case StepType::eKeyDown:
                pushKey(pWindow, tStep.text, KeyPhase::eDown);
                return;
            case StepType::eKeyUp:
                pushKey(pWindow, tStep.text, KeyPhase::eUp);
                return;
            case StepType::eQuit:
                mNextStep = mSteps.size();
                mLiveDir.clear();
                break;
            case StepType::eText:
                pushText(pWindow, tStep.text);
                return;
            case StepType::eMouseMove:
            case StepType::eMouseDown:
            case StepType::eMouseUp:
            case StepType::eMouseDrag:
            case StepType::eWheel:
                pushMouse(pWindow, tStep);
                return;
            case StepType::eShot:
                saveShot(pWindow, tStep.text);
                break;
        }
    }

    releaseHeldKeys(pWindow);

    if (!mLiveDir.empty())
    {
        return;
    }

    mIsFinished = true;

    SDL_Event tQuit{};
    tQuit.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&tQuit);
}

void SnapshotRunner::updateLive()
{
    if (!mLiveCommandName.empty())
    {
        finishLiveCommand(mLiveCommandName);
        mLiveCommandName.clear();
    }

    char tName[32];
    std::snprintf(tName, sizeof(tName), "cmd-%04d.txt", mLiveIndex);
    std::filesystem::path tPath = std::filesystem::path(mLiveDir) / tName;

    if (!std::filesystem::exists(tPath))
    {
        return;
    }

    mSteps.clear();
    mNextStep = 0;
    mIgnoredLines = 0;
    mLiveCommandName = tName;
    ++mLiveIndex;
    loadScript(tPath.string());
}

void SnapshotRunner::finishLiveCommand(const std::string& pName)
{
    std::string tDoneName = "done-" + pName.substr(4);
    std::ofstream tDone(std::filesystem::path(mLiveDir) / tDoneName);
    tDone << (mIgnoredLines == 0 ? "ok" : "ignored " + std::to_string(mIgnoredLines)) << "\n";
}

void SnapshotRunner::pushKey(SDL_Window* pWindow, const std::string& pName, KeyPhase pPhase)
{
    SDL_Keycode tKey = resolveKey(pName);

    if (tKey == SDLK_UNKNOWN)
    {
        SDL_Log("Snapshot script key unknown: %s", pName.c_str());
        return;
    }

    if (pPhase == KeyPhase::eDown)
    {
        mHeldKeys.insert(pName);
    }
    else
    {
        mHeldKeys.erase(pName);
    }

    SDL_Event tEvent{};
    tEvent.key.windowID = SDL_GetWindowID(pWindow);
    tEvent.key.key = tKey;
    SDL_Scancode tScancode = SDL_GetScancodeFromName(pName == "esc" ? "Escape" : pName == "enter" ? "Return" : pName.c_str());
    tEvent.key.scancode = tScancode != SDL_SCANCODE_UNKNOWN ? tScancode : SDL_GetScancodeFromKey(tKey, nullptr);

    if (pPhase != KeyPhase::eUp)
    {
        tEvent.type = SDL_EVENT_KEY_DOWN;
        tEvent.key.down = true;
        SDL_PushEvent(&tEvent);
    }

    if (pPhase != KeyPhase::eDown)
    {
        tEvent.type = SDL_EVENT_KEY_UP;
        tEvent.key.down = false;
        SDL_PushEvent(&tEvent);
    }
}

void SnapshotRunner::releaseHeldKeys(SDL_Window* pWindow)
{
    std::set<std::string> tHeld;
    tHeld.swap(mHeldKeys);

    for (const std::string& tName : tHeld)
    {
        SDL_Log("snapshot released held key %s", tName.c_str());
        pushKey(pWindow, tName, KeyPhase::eUp);
    }
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

    if (pStep.type == StepType::eMouseDrag)
    {
        const float tCenterX = static_cast<float>(tWidth) * 0.5f;
        const float tCenterY = static_cast<float>(tHeight) * 0.5f;

        tEvent.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        tEvent.button.windowID = tWindowId;
        tEvent.button.button = SDL_BUTTON_RIGHT;
        tEvent.button.down = true;
        tEvent.button.clicks = 1;
        tEvent.button.x = tCenterX;
        tEvent.button.y = tCenterY;
        SDL_PushEvent(&tEvent);

        tEvent = SDL_Event{};
        tEvent.type = SDL_EVENT_MOUSE_MOTION;
        tEvent.motion.windowID = tWindowId;
        tEvent.motion.x = tCenterX;
        tEvent.motion.y = tCenterY;
        tEvent.motion.xrel = pStep.x;
        tEvent.motion.yrel = pStep.y;
        SDL_PushEvent(&tEvent);

        tEvent = SDL_Event{};
        tEvent.type = SDL_EVENT_MOUSE_BUTTON_UP;
        tEvent.button.windowID = tWindowId;
        tEvent.button.button = SDL_BUTTON_RIGHT;
        tEvent.button.down = false;
        tEvent.button.clicks = 1;
        tEvent.button.x = tCenterX;
        tEvent.button.y = tCenterY;
    }
    else if (pStep.type == StepType::eWheel)
    {
        tEvent.type = SDL_EVENT_MOUSE_WHEEL;
        tEvent.wheel.windowID = tWindowId;
        tEvent.wheel.y = pStep.y;
    }
    else if (pStep.type == StepType::eMouseMove)
    {
        tEvent.type = SDL_EVENT_MOUSE_MOTION;
        tEvent.motion.windowID = tWindowId;
        tEvent.motion.x = tX;
        tEvent.motion.y = tY;
        tEvent.motion.xrel = (pStep.x - mMouseX) * static_cast<float>(tWidth);
        tEvent.motion.yrel = (pStep.y - mMouseY) * static_cast<float>(tHeight);
        mMouseX = pStep.x;
        mMouseY = pStep.y;
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
