#ifndef LAKOT_MINIMAP_RML_CONTROLLER_H
#define LAKOT_MINIMAP_RML_CONTROLLER_H

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>

namespace lakot
{

class RmlUiLayer;

// The always-on minimap in the top-right corner: a north-up window onto the
// map image centred on the player, with an arrow for the player and a dot for
// everyone else in range.
class MinimapRmlController
{
public:
    explicit MinimapRmlController(RmlUiLayer& pRmlUiLayer);
    ~MinimapRmlController();

    MinimapRmlController(const MinimapRmlController&) = delete;
    MinimapRmlController& operator=(const MinimapRmlController&) = delete;

    // pImageName was registered with RmlUiLayer::registerGeneratedImage.
    void setMap(const std::string& pImageName, const std::string& pMapName, float pWorldSize);

    // pFacingDegrees: 0 = north, clockwise.
    void update(const glm::vec3& pPlayerPosition, float pFacingDegrees, const std::vector<glm::vec3>& pOtherPlayers);

private:
    struct PlayerDot
    {
        Rml::String left{"0px"};
        Rml::String top{"0px"};
    };

    RmlUiLayer& mRmlUiLayer;

    Rml::ElementDocument* mDocument{nullptr};
    Rml::DataModelHandle mModelHandle;

    float mWorldSize{1.0f};

    Rml::String mMapSource;
    Rml::String mMapName;
    // Valid before the first setMap()/update(): the document binds these
    // straight into styles, and an empty value is an RmlUi syntax error.
    Rml::String mMapSize{"0px"};
    Rml::String mMapLeft{"0px"};
    Rml::String mMapTop{"0px"};
    Rml::String mArrowTransform{"rotate(0deg)"};
    Rml::String mCoordinates;
    std::vector<PlayerDot> mPlayers;

    // Sets pTarget and marks it dirty only when the value actually changed.
    void assign(Rml::String& pTarget, const Rml::String& pValue, const char* pName);
};

}

#endif
