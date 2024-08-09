#ifndef LAKOT_AWINDOW_H
#define LAKOT_AWINDOW_H

#include <lakot/interface/graphics/window/IWindow.h>

namespace lakot {

class AWindow : public IWindow
{
public:
    virtual ~AWindow() override;
    AWindow();

    virtual void update() override = 0;
    virtual void nextFrame() override = 0;

    bool getIsInitialized() const override;

    bool getIsActive() const override;

    unsigned int getWidth() const override;
    void setWidth(unsigned int pWidth) override;

    unsigned int getHeight() const override;
    void setHeight(unsigned int pHeight) override;

    const std::string& getTitle() const override;
    void setTitle(const std::string& pTitle) override;

    double getTimeDifference() const override;
    double getFPS() const override;

    void setOnResizeFunction(const std::function<void (double, double)>& pOnResizeFunction) override;

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
