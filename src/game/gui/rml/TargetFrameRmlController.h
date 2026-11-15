#ifndef LAKOT_TARGETFRAMERMLCONTROLLER_H
#define LAKOT_TARGETFRAMERMLCONTROLLER_H

#include <cstdint>
#include <string>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

#include "RmlDocument.h"

namespace lakot
{

class RmlUiLayer;

class TargetFrameRmlController
{
public:
    explicit TargetFrameRmlController(RmlUiLayer& pRmlUiLayer);
    ~TargetFrameRmlController();

    TargetFrameRmlController(const TargetFrameRmlController&) = delete;
    TargetFrameRmlController& operator=(const TargetFrameRmlController&) = delete;

    void setTarget(const std::string& pName, uint32_t pLevel, uint32_t pHealthPercent);
    void clear();

private:
    RmlUiLayer& mRmlUiLayer;
    RmlDocument mDocument;
    Rml::DataModelHandle mModel;

    Rml::String mName;
    Rml::String mLevel;
    Rml::String mHealthPercent{"100%"};
    bool mIsVisible{false};
};

}

#endif
