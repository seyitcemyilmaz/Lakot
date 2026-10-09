#include "DisplaySettings.h"

#include <algorithm>
#include <cstdio>
#include <fstream>

using namespace lakot;

namespace
{
    constexpr int kWindowedSizes[][2] = { { 1920, 1080 }, { 1600, 900 }, { 1366, 768 }, { 1280, 720 } };

    const DisplayMode kFullscreen{ DisplayModeType::eFullscreen, 0, 0 };
    const DisplayMode kDefaultWindowed{ DisplayModeType::eWindowed, 1280, 720 };

    bool isAvailable(const std::vector<DisplayMode>& pModes, const DisplayMode& pMode)
    {
        return std::find(pModes.begin(), pModes.end(), pMode) != pModes.end();
    }
}

bool DisplayMode::operator==(const DisplayMode& pOther) const
{
    if (type != pOther.type)
    {
        return false;
    }

    return type == DisplayModeType::eFullscreen || (width == pOther.width && height == pOther.height);
}

std::string DisplayMode::getLabel() const
{
    std::string tSize = std::to_string(width) + " x " + std::to_string(height);

    if (type == DisplayModeType::eFullscreen)
    {
        return width > 0 ? "Fullscreen (" + tSize + ")" : "Fullscreen";
    }

    return tSize;
}

void DisplaySettings::load(SDL_Window* pWindow)
{
    std::vector<DisplayMode> tModes = getAvailableModes(pWindow);

    mMode = isAvailable(tModes, kDefaultWindowed) ? kDefaultWindowed : kFullscreen;

    std::ifstream tFile(getFilePath());
    std::string tLine;

    while (std::getline(tFile, tLine))
    {
        if (tLine.rfind("display=", 0) != 0)
        {
            continue;
        }

        std::string tValue = tLine.substr(8);
        DisplayMode tMode;

        if (tValue == "fullscreen")
        {
            tMode = kFullscreen;
        }
        else if (std::sscanf(tValue.c_str(), "%dx%d", &tMode.width, &tMode.height) == 2)
        {
            tMode.type = DisplayModeType::eWindowed;
        }
        else
        {
            continue;
        }

        if (isAvailable(tModes, tMode))
        {
            mMode = tMode;
        }
    }
}

void DisplaySettings::save() const
{
    std::ofstream tFile(getFilePath(), std::ios::trunc);

    if (!tFile)
    {
        SDL_Log("Settings could not be saved to %s", getFilePath().c_str());
        return;
    }

    if (mMode.type == DisplayModeType::eFullscreen)
    {
        tFile << "display=fullscreen\n";
    }
    else
    {
        tFile << "display=" << mMode.width << "x" << mMode.height << "\n";
    }
}

void DisplaySettings::apply(SDL_Window* pWindow) const
{
    if (mMode.type == DisplayModeType::eFullscreen)
    {
        SDL_SetWindowFullscreenMode(pWindow, nullptr);
        SDL_SetWindowFullscreen(pWindow, true);
    }
    else
    {
        SDL_SetWindowFullscreen(pWindow, false);
        SDL_RestoreWindow(pWindow);
        SDL_SetWindowSize(pWindow, mMode.width, mMode.height);
        SDL_SetWindowPosition(pWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }

    SDL_SyncWindow(pWindow);
}

void DisplaySettings::change(SDL_Window* pWindow, const DisplayMode& pMode)
{
    mMode = pMode;
    apply(pWindow);
    save();
}

const DisplayMode& DisplaySettings::getMode() const
{
    return mMode;
}

void DisplaySettings::setMode(const DisplayMode& pMode)
{
    mMode = pMode;
}

std::vector<DisplayMode> DisplaySettings::getAvailableModes(SDL_Window* pWindow)
{
    SDL_DisplayID tDisplay = SDL_GetDisplayForWindow(pWindow);

    DisplayMode tFullscreen = kFullscreen;

    if (const SDL_DisplayMode* tDesktop = SDL_GetDesktopDisplayMode(tDisplay))
    {
        tFullscreen.width = tDesktop->w;
        tFullscreen.height = tDesktop->h;
    }

    std::vector<DisplayMode> tModes{ tFullscreen };

    SDL_Rect tUsable{};

    if (!SDL_GetDisplayUsableBounds(tDisplay, &tUsable))
    {
        tModes.push_back(kDefaultWindowed);
        return tModes;
    }

    for (const auto& tSize : kWindowedSizes)
    {
        if (tSize[0] <= tUsable.w && tSize[1] <= tUsable.h)
        {
            tModes.push_back({ DisplayModeType::eWindowed, tSize[0], tSize[1] });
        }
    }

    return tModes;
}

std::string DisplaySettings::getFilePath()
{
    std::string tPath;

    if (char* tPrefPath = SDL_GetPrefPath("Lakot", "LakotGame"))
    {
        tPath = tPrefPath;
        SDL_free(tPrefPath);
    }

    return tPath + "settings.cfg";
}
