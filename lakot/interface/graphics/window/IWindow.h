#ifndef LAKOT_IWINDOW_H
#define LAKOT_IWINDOW_H

#include <string>
#include <functional>

#include "../../engine/IObject.h"

namespace lakot {

class IWindow : public IObject
{
public:
    virtual ~IWindow() = default;
    IWindow() = default;

    virtual void update() = 0;
    virtual void nextFrame() = 0;

    virtual bool getIsInitialized() const = 0;

    virtual bool getIsActive() const = 0;

    virtual unsigned int getWidth() const = 0;
    virtual void setWidth(unsigned int pWidth) = 0;

    virtual unsigned int getHeight() const = 0;
    virtual void setHeight(unsigned int pHeight) = 0;

    virtual const std::string& getTitle() const = 0;
    virtual void setTitle(const std::string& pTitle) = 0;

    virtual double getTimeDifference() const = 0;
    virtual double getFPS() const = 0;

    virtual void setOnResizeFunction(const std::function<void (double, double)>& pOnResizeFunction) = 0;
};

}

#endif
