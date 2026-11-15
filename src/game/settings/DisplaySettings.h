#ifndef LAKOT_DISPLAY_SETTINGS_H
#define LAKOT_DISPLAY_SETTINGS_H

#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "../localization/Localization.h"

namespace lakot
{

enum class DisplayModeType
{
    eFullscreen,
    eWindowed
};

struct DisplayMode
{
    DisplayModeType type = DisplayModeType::eWindowed;
    int width = 0;
    int height = 0;

    bool operator==(const DisplayMode& pOther) const;

    std::string getLabel(const Localization& pLocalization) const;
};

class DisplaySettings
{
public:
    void load(SDL_Window* pWindow);
    void save() const;

    void apply(SDL_Window* pWindow) const;
    void applyVsync() const;

    void change(SDL_Window* pWindow, const DisplayMode& pMode);

    const DisplayMode& getMode() const;
    void setMode(const DisplayMode& pMode);

    bool isCameraShakeEnabled() const;
    void setCameraShakeEnabled(bool pIsEnabled);

    bool isVsyncEnabled() const;
    void setVsyncEnabled(bool pIsEnabled);

    size_t getViewDistanceLevel() const;
    void setViewDistanceLevel(size_t pLevel);
    float getViewDistance() const;

    static size_t getViewDistanceLevelCount();
    static float getViewDistanceScale(size_t pLevel);

    static std::vector<DisplayMode> getAvailableModes(SDL_Window* pWindow);

private:
    DisplayMode mMode;
    bool mIsCameraShakeEnabled{true};
    bool mIsVsyncEnabled{true};
    size_t mViewDistanceLevel{2};

    static std::string getFilePath();
};

}

#endif
