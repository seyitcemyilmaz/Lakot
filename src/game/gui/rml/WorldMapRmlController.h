#ifndef LAKOT_WORLD_MAP_RML_CONTROLLER_H
#define LAKOT_WORLD_MAP_RML_CONTROLLER_H

#include <string>

#include <glm/glm.hpp>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>

namespace lakot
{

class RmlUiLayer;

// The whole current map, toggled with M, with the player's position on it.
class WorldMapRmlController
{
public:
    explicit WorldMapRmlController(RmlUiLayer& pRmlUiLayer);
    ~WorldMapRmlController();

    WorldMapRmlController(const WorldMapRmlController&) = delete;
    WorldMapRmlController& operator=(const WorldMapRmlController&) = delete;

    // pImageName was registered with RmlUiLayer::registerGeneratedImage.
    void setMap(const std::string& pImageName, const std::string& pMapName, float pWorldSize);

    // pFacingDegrees: 0 = north, clockwise.
    void update(const glm::vec3& pPlayerPosition, float pFacingDegrees);

    void toggle();
    void close();
    bool isOpen() const;

private:
    RmlUiLayer& mRmlUiLayer;

    Rml::ElementDocument* mDocument{nullptr};
    Rml::DataModelHandle mModelHandle;

    float mWorldSize{1.0f};
    bool mIsOpen{false};

    Rml::String mMapSource;
    Rml::String mMapName;
    // Valid before the first update(), for the same reason as the minimap's.
    Rml::String mMarkerLeft{"50%"};
    Rml::String mMarkerTop{"50%"};
    Rml::String mArrowTransform{"rotate(0deg)"};

    // Moving the window by its titlebar - same mechanism as the bag.
    bool mIsDragging{false};
    float mDragStartMouseX{0.0f};
    float mDragStartMouseY{0.0f};
    float mDragStartDocLeft{0.0f};
    float mDragStartDocTop{0.0f};

    void onCloseClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onDragStart(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);

    void updateDrag();

    void assign(Rml::String& pTarget, const Rml::String& pValue, const char* pName);
};

}

#endif
