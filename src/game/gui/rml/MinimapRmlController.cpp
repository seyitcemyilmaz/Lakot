#include "MinimapRmlController.h"

#include <cmath>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/DataStructHandle.h>

#include "RmlUiLayer.h"

using namespace lakot;

namespace
{
    // Must match .minimap-view in theme.rcss.
    constexpr float kViewPixels = 170.0f;

    // How much of the world the view spans - a little more than the 150-unit
    // area of interest, so everyone the server reports is on it.
    constexpr float kViewWorldUnits = 200.0f;

    constexpr float kPixelsPerUnit = kViewPixels / kViewWorldUnits;

    // Must match .minimap-dot's size.
    constexpr float kDotRadius = 3.0f;

    Rml::String toDp(float pValue)
    {
        return Rml::String(std::to_string(pValue) + "dp");
    }
}

MinimapRmlController::MinimapRmlController(RmlUiLayer& pRmlUiLayer)
    : mRmlUiLayer(pRmlUiLayer)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("minimap");

    // RmlUi's struct registry is per context, not per model, and this
    // controller is rebuilt on every visit to the world.
    static bool sIsDotTypeRegistered = false;

    if (!sIsDotTypeRegistered)
    {
        Rml::StructHandle<PlayerDot> tDotHandle = tConstructor.RegisterStruct<PlayerDot>();
        tDotHandle.RegisterMember("left", &PlayerDot::left);
        tDotHandle.RegisterMember("top", &PlayerDot::top);
        tConstructor.RegisterArray<std::vector<PlayerDot>>();

        sIsDotTypeRegistered = true;
    }

    tConstructor.Bind("map_source", &mMapSource);
    tConstructor.Bind("map_name", &mMapName);
    tConstructor.Bind("map_size", &mMapSize);
    tConstructor.Bind("map_left", &mMapLeft);
    tConstructor.Bind("map_top", &mMapTop);
    tConstructor.Bind("arrow_transform", &mArrowTransform);
    tConstructor.Bind("coordinates", &mCoordinates);
    tConstructor.Bind("players", &mPlayers);

    mModelHandle = tConstructor.GetModelHandle();

    mDocument = mRmlUiLayer.loadDocument("ui/minimap.rml");

    if (mDocument)
    {
        mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
    }
}

MinimapRmlController::~MinimapRmlController()
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        if (mDocument)
        {
            tContext->UnloadDocument(mDocument);
        }

        tContext->RemoveDataModel("minimap");
    }
}

void MinimapRmlController::setMap(const std::string& pImageName, const std::string& pMapName, float pWorldSize)
{
    mWorldSize = pWorldSize;

    assign(mMapSource, "lakot-generated/" + pImageName, "map_source");
    assign(mMapName, pMapName, "map_name");
    assign(mMapSize, toDp(pWorldSize * kPixelsPerUnit), "map_size");
}

void MinimapRmlController::update(const glm::vec3& pPlayerPosition, float pFacingDegrees, const std::vector<glm::vec3>& pOtherPlayers)
{
    const float tHalfWorld = mWorldSize * 0.5f;
    const float tCentre = kViewPixels * 0.5f;

    // The image is slid so the player's point on it sits in the middle.
    assign(mMapLeft, toDp(tCentre - (pPlayerPosition.x + tHalfWorld) * kPixelsPerUnit), "map_left");
    assign(mMapTop, toDp(tCentre - (pPlayerPosition.z + tHalfWorld) * kPixelsPerUnit), "map_top");

    assign(mArrowTransform, "rotate(" + std::to_string(static_cast<int>(std::lround(pFacingDegrees))) + "deg)", "arrow_transform");

    assign(mCoordinates, std::to_string(static_cast<int>(std::lround(pPlayerPosition.x))) + ", "
                         + std::to_string(static_cast<int>(std::lround(pPlayerPosition.z))), "coordinates");

    std::vector<PlayerDot> tDots;
    tDots.reserve(pOtherPlayers.size());

    for (const glm::vec3& tOther : pOtherPlayers)
    {
        float tX = tCentre + (tOther.x - pPlayerPosition.x) * kPixelsPerUnit;
        float tY = tCentre + (tOther.z - pPlayerPosition.z) * kPixelsPerUnit;

        if (tX < 0.0f || tY < 0.0f || tX > kViewPixels || tY > kViewPixels)
        {
            continue;
        }

        tDots.push_back({ toDp(tX - kDotRadius), toDp(tY - kDotRadius) });
    }

    bool tIsSame = tDots.size() == mPlayers.size();

    for (size_t tIndex = 0; tIsSame && tIndex < tDots.size(); ++tIndex)
    {
        tIsSame = tDots[tIndex].left == mPlayers[tIndex].left && tDots[tIndex].top == mPlayers[tIndex].top;
    }

    if (!tIsSame)
    {
        mPlayers = std::move(tDots);
        mModelHandle.DirtyVariable("players");
    }
}

void MinimapRmlController::assign(Rml::String& pTarget, const Rml::String& pValue, const char* pName)
{
    if (pTarget != pValue)
    {
        pTarget = pValue;
        mModelHandle.DirtyVariable(pName);
    }
}
