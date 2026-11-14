#ifndef LAKOT_SNAPSHOTRUNNER_H
#define LAKOT_SNAPSHOTRUNNER_H

#if defined(LAKOT_DEV_TOOLS)

#include <cstdint>
#include <iosfwd>
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
        eKeyDown,
        eKeyUp,
        eQuit,
        eText,
        eMouseMove,
        eMouseDown,
        eMouseUp,
        eMouseDrag,
        eWheel,
        eShot
    };

    enum class KeyPhase
    {
        eTap,
        eDown,
        eUp
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

    void addOrbit(float pDeltaX, float pDeltaY);
    void addKeyHold(const std::string& pName, int pMs);
    void addDrag(float pX1, float pY1, float pX2, float pY2, int pMs, int pButton);
    bool parseDrag(std::istringstream& pStream);
    bool loadScript(const std::string& pPath);
    void updateLive();
    void finishLiveCommand(const std::string& pName);
    void addClick(float pX, float pY, int pButton, int pCount);
    bool parseLine(const std::string& pLine);
    void pushKey(SDL_Window* pWindow, const std::string& pName, KeyPhase pPhase);
    void pushText(SDL_Window* pWindow, const std::string& pText);
    void pushMouse(SDL_Window* pWindow, const Step& pStep);
    void saveShot(SDL_Window* pWindow, const std::string& pName);

    std::vector<Step> mSteps;
    std::string mOutDir;
    std::string mLiveDir;
    std::string mLiveCommandName;
    int mLiveIndex{1};
    int mIgnoredLines{0};
    float mMouseX{0.5f};
    float mMouseY{0.5f};

    size_t mNextStep{0};
    uint64_t mWaitUntilMs{0};
    int mWaitFrames{0};
    bool mIsFinished{false};
};

}

#endif

#endif
