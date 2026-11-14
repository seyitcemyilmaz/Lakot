#ifndef LAKOT_SNAPSHOTRUNNER_H
#define LAKOT_SNAPSHOTRUNNER_H

#if defined(LAKOT_DEV_TOOLS)

#include <cstdint>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

namespace lakot
{

class SnapshotRunner
{
public:
    static bool isRequested();
    static int getRequestedWidth();
    static int getRequestedHeight();

    bool load(const std::string& pScriptPath, const std::string& pOutDir);

    void update(SDL_Window* pWindow);

private:
    enum class StepType
    {
        eWaitMs,
        eWaitFrames,
        eKey,
        eText,
        eMouseMove,
        eMouseDown,
        eMouseUp,
        eShot
    };

    struct Step
    {
        StepType type{StepType::eWaitFrames};
        int number{0};
        float x{0.0f};
        float y{0.0f};
        int button{SDL_BUTTON_LEFT};
        std::string text;
    };

    void addClick(float pX, float pY, int pButton, int pCount);
    bool parseLine(const std::string& pLine);
    void pushKey(SDL_Window* pWindow, const std::string& pName);
    void pushText(SDL_Window* pWindow, const std::string& pText);
    void pushMouse(SDL_Window* pWindow, const Step& pStep);
    void saveShot(SDL_Window* pWindow, const std::string& pName);

    std::vector<Step> mSteps;
    std::string mOutDir;

    size_t mNextStep{0};
    uint64_t mWaitUntilMs{0};
    int mWaitFrames{0};
    bool mIsFinished{false};
};

}

#endif

#endif
