#include "WorldMapRmlController.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <SDL3/SDL_mouse.h>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>

#include "RmlUiLayer.h"

using namespace lakot;

namespace
{
    Rml::String toPercent(float pFraction)
    {
        char tBuffer[32];
        std::snprintf(tBuffer, sizeof(tBuffer), "%.2f%%", std::clamp(pFraction, 0.0f, 1.0f) * 100.0f);
        return Rml::String(tBuffer);
    }
}

WorldMapRmlController::WorldMapRmlController(RmlUiLayer& pRmlUiLayer)
    : mRmlUiLayer(pRmlUiLayer)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("worldmap");

    tConstructor.Bind("map_source", &mMapSource);
    tConstructor.Bind("map_name", &mMapName);
    tConstructor.Bind("marker_left", &mMarkerLeft);
    tConstructor.Bind("marker_top", &mMarkerTop);
    tConstructor.Bind("arrow_transform", &mArrowTransform);

    tConstructor.BindEventCallback("close", &WorldMapRmlController::onCloseClicked, this);
    tConstructor.BindEventCallback("drag_start", &WorldMapRmlController::onDragStart, this);

    mModelHandle = tConstructor.GetModelHandle();

    mDocument = mRmlUiLayer.loadDocument("ui/worldmap.rml");

    // Loaded but not shown - M toggles it.
}

WorldMapRmlController::~WorldMapRmlController()
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        if (mDocument)
        {
            tContext->UnloadDocument(mDocument);
        }

        tContext->RemoveDataModel("worldmap");
    }
}

void WorldMapRmlController::setMap(const std::string& pImageName, const std::string& pMapName, float pWorldSize)
{
    mWorldSize = pWorldSize;

    assign(mMapSource, "lakot-generated/" + pImageName, "map_source");
    assign(mMapName, pMapName, "map_name");
}

void WorldMapRmlController::update(const glm::vec3& pPlayerPosition, float pFacingDegrees)
{
    if (!mIsOpen)
    {
        return;
    }

    updateDrag();

    const float tHalfWorld = mWorldSize * 0.5f;

    assign(mMarkerLeft, toPercent((pPlayerPosition.x + tHalfWorld) / mWorldSize), "marker_left");
    assign(mMarkerTop, toPercent((pPlayerPosition.z + tHalfWorld) / mWorldSize), "marker_top");
    assign(mArrowTransform, "rotate(" + std::to_string(static_cast<int>(std::lround(pFacingDegrees))) + "deg)", "arrow_transform");
}

void WorldMapRmlController::toggle()
{
    if (mIsOpen)
    {
        close();
        return;
    }

    mIsOpen = true;

    if (mDocument)
    {
        // FocusFlag::None, like the bag - opening the map must not take
        // keyboard focus away from movement.
        mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
        mDocument->PullToFront();
    }
}

void WorldMapRmlController::close()
{
    if (!mIsOpen)
    {
        return;
    }

    mIsOpen = false;

    if (mDocument)
    {
        mDocument->Hide();
    }
}

bool WorldMapRmlController::isOpen() const
{
    return mIsOpen;
}

void WorldMapRmlController::onCloseClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    close();
}

void WorldMapRmlController::onDragStart(Rml::DataModelHandle /*pHandle*/, Rml::Event& pEvent, const Rml::VariantList& /*pArguments*/)
{
    if (pEvent.GetParameter<int>("button", -1) != 0 || !mDocument)
    {
        return;
    }

    mIsDragging = true;
    SDL_GetMouseState(&mDragStartMouseX, &mDragStartMouseY);
    mDragStartDocLeft = mDocument->GetAbsoluteLeft();
    mDragStartDocTop = mDocument->GetAbsoluteTop();
}

void WorldMapRmlController::updateDrag()
{
    if (!mIsDragging || !mDocument)
    {
        return;
    }

    float tMouseX = 0.0f;
    float tMouseY = 0.0f;

    if (!(SDL_GetMouseState(&tMouseX, &tMouseY) & SDL_BUTTON_LMASK))
    {
        mIsDragging = false;
        return;
    }

    // The document is the full-window body the window is centred in, so
    // moving it moves the window.
    Rml::Vector2f tClamped = mRmlUiLayer.clampDocumentToScreen(mDocument, mDocument->QuerySelector(".worldmap-window"),
        Rml::Vector2f(mDragStartDocLeft + (tMouseX - mDragStartMouseX), mDragStartDocTop + (tMouseY - mDragStartMouseY)));

    mDocument->SetProperty("left", std::to_string(tClamped.x) + "px");
    mDocument->SetProperty("top", std::to_string(tClamped.y) + "px");
}

void WorldMapRmlController::assign(Rml::String& pTarget, const Rml::String& pValue, const char* pName)
{
    if (pTarget != pValue)
    {
        pTarget = pValue;
        mModelHandle.DirtyVariable(pName);
    }
}
