#include "TargetFrameRmlController.h"

#include <algorithm>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include "RmlUiLayer.h"

using namespace lakot;

TargetFrameRmlController::TargetFrameRmlController(RmlUiLayer& pRmlUiLayer)
    : mRmlUiLayer(pRmlUiLayer)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (!tContext)
    {
        return;
    }

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("target_frame");
    tConstructor.Bind("name", &mName);
    tConstructor.Bind("level", &mLevel);
    tConstructor.Bind("health_percent", &mHealthPercent);
    tConstructor.Bind("is_visible", &mIsVisible);
    mModel = tConstructor.GetModelHandle();

    mDocument.load(mRmlUiLayer, "ui/target_frame.rml");

    if (mDocument)
    {
        mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
    }
}

TargetFrameRmlController::~TargetFrameRmlController()
{
    mDocument.reset();

    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        tContext->RemoveDataModel("target_frame");
    }
}

void TargetFrameRmlController::setTarget(const std::string& pName, uint32_t pLevel, uint32_t pHealthPercent)
{
    Rml::String tLevel = std::to_string(pLevel);
    Rml::String tPercent = std::to_string(std::min<uint32_t>(pHealthPercent, 100)) + "%";

    if (pName != mName)
    {
        mName = pName;
        mModel.DirtyVariable("name");
    }

    if (tLevel != mLevel)
    {
        mLevel = tLevel;
        mModel.DirtyVariable("level");
    }

    if (tPercent != mHealthPercent)
    {
        mHealthPercent = tPercent;
        mModel.DirtyVariable("health_percent");
    }

    if (!mIsVisible)
    {
        mIsVisible = true;
        mModel.DirtyVariable("is_visible");
    }
}

void TargetFrameRmlController::clear()
{
    if (mIsVisible)
    {
        mIsVisible = false;
        mModel.DirtyVariable("is_visible");
    }
}
