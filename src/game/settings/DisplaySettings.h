#ifndef LAKOT_DISPLAY_SETTINGS_H
#define LAKOT_DISPLAY_SETTINGS_H

#include <string>
#include <vector>

#include <SDL3/SDL.h>

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

    std::string getLabel() const;
};

class DisplaySettings
{
public:
    void load(SDL_Window* pWindow);
    void save() const;

    void apply(SDL_Window* pWindow) const;

    void change(SDL_Window* pWindow, const DisplayMode& pMode);

    const DisplayMode& getMode() const;
    void setMode(const DisplayMode& pMode);

    static std::vector<DisplayMode> getAvailableModes(SDL_Window* pWindow);

private:
    DisplayMode mMode;

    static std::string getFilePath();
};

}

#endif
