#ifndef LAKOT_AWINDOW_H
#define LAKOT_AWINDOW_H

#include <functional>

#include <lakot/utilities/Object.h>
#include <lakot/utilities/Type.h>

namespace lakot {

class AWindow : public Object
{
public:
    virtual ~AWindow() override;
    AWindow();

    virtual void update() = 0;
    virtual void nextFrame() = 0;

    bool getIsInitialized() const;

    bool getIsActive() const;

    unsigned int getWidth() const;
    void setWidth(unsigned int pWidth);

    unsigned int getHeight() const;
    void setHeight(unsigned int pHeight);

    const std::string& getTitle() const;
    void setTitle(const std::string& pTitle);

    double getTimeDifference() const;
    double getFPS() const;

    void setOnResizeFunction(const std::function<void (double, double)>& pOnResizeFunction);

protected:
    bool mIsInitialized;

    bool mIsActive;

    unsigned int mWidth;
    unsigned int mHeight;

    std::string mTitle;

    double mTimeDifference;
    double mFPS;

    double mCurrentTime;
    double mPreviousTime;

    std::function<void (double, double)> mOnResizeFunction;
};

}

#endif
