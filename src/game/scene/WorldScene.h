#ifndef LAKOT_WORLDSCENE_H
#define LAKOT_WORLDSCENE_H

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include <imgui.h>

#include "Scene.h"

#include "../graphics/camera/ThirdPersonCamera.h"

#include "../graphics/geometry/BoxContainer.h"
#include "../graphics/geometry/Terrain.h"

#include "../network/controller/WorldController.h"

#include "../gui/panels/ChatPanel.h"

namespace lakot
{

class WorldScene final : public Scene
{
public:
    // Restored account state (see AccountRepository/AuthController) to spawn
    // at instead of FPSCamera's built-in default - yaw is in radians,
    // matching the wire protocol convention used everywhere else.
    struct InitialSpawnState
    {
        uint32_t mapId;
        float x;
        float y;
        float z;
        float yaw;
    };

    virtual ~WorldScene() override;
    WorldScene(Engine& pEngine, const std::optional<InitialSpawnState>& pSpawnState = std::nullopt, const std::string& pUsername = "Player");

    void enter() override;

    void exit() override;

    void update(double pDeltaTime) override;

    void render() override;

    bool handleEvent(SDL_Event* pEvent) override;

private:
    std::unique_ptr<BoxContainer> mBoxContainer;
    std::unique_ptr<Terrain> mTerrain;

    std::unique_ptr<ThirdPersonCamera> mCamera;

    // The window's actual framebuffer size in physical pixels, refreshed
    // every render() - this is what mCamera's projection aspect ratio is
    // built from, so renderNameplate() must map NDC using these exact same
    // dimensions (converted into ImGui's coordinate space afterward) rather
    // than ImGui::GetIO().DisplaySize independently. The two can disagree
    // under per-monitor DPI scaling, which put nameplates in the wrong spot
    // (or off-screen entirely) on displays where the scale factor isn't 1.
    int mViewportWidth{1};
    int mViewportHeight{1};

    // The local player's own visible avatar - first-person needed none, the
    // follow camera does. Same placeholder-box treatment as remote players.
    std::unique_ptr<BoxContainer> mLocalCharacterBox;

    bool mIsMovingForward{false};
    bool mIsMovingBackward{false};
    bool mIsMovingLeft{false};
    bool mIsMovingRight{false};

    // True only while the right mouse button is held - orbiting (and the
    // relative/captured mouse mode that goes with it) is gated on this.
    bool mIsOrbiting{false};

    // A right-click is a camera orbit if the mouse moves while held, or a
    // target pick (context menu) if it doesn't - mOrbitDragDistance
    // accumulates total mouse motion since the button went down, in window
    // pixels, and mRightClickDownPos remembers where to pick from and where
    // to open the menu if it turns out to be a click.
    glm::vec2 mRightClickDownPos{0.0f, 0.0f};
    float mOrbitDragDistance{0.0f};

    // What a right-click landed on - only ePlayer exists today, but this is
    // the extension point for right-clicking monsters later, per the user's
    // request, without reshaping the picking/menu code again.
    enum class TargetKind
    {
        ePlayer
    };

    struct TargetInfo
    {
        TargetKind kind;
        uint64_t id;
        std::string name;
    };

    // The target menu is a single ImGui popup ("TargetContextMenu") that
    // stays open across frames once triggered - mContextMenuOpenRequested is
    // a one-frame edge (consumed by render() to call ImGui::OpenPopup
    // exactly once), while mContextMenuTarget is retained for as long as
    // the popup itself stays open, to render its contents.
    bool mContextMenuOpenRequested{false};
    glm::vec2 mContextMenuScreenPos{0.0f, 0.0f};
    std::optional<TargetInfo> mContextMenuTarget;

    // A remote player's rendered position glides from previousPosition to
    // targetPosition over kInterpolationDuration seconds instead of snapping
    // straight to each network update - state updates arrive in discrete
    // ~100ms steps (see kStateSendInterval) but the screen still redraws at
    // full frame rate, so without this the box visibly teleports.
    struct RemotePlayerVisual
    {
        glm::vec3 previousPosition;
        glm::vec3 targetPosition;
        float yaw;
        double interpolationElapsed;
        std::string username;
    };

    // Other connected players, as reported by WorldController - rendered as
    // placeholder boxes, separate from the static decor in mBoxContainer.
    std::unique_ptr<BoxContainer> mRemotePlayerBoxes;
    std::unordered_map<uint64_t, RemotePlayerVisual> mRemotePlayers;

    glm::vec3 mLastSentPosition{0.0f, 0.0f, 0.0f};
    float mLastSentYaw{0.0f};
    bool mHasSentInitialState{false};
    double mStateSendAccumulator{0.0};

    // Matches services::world::MapId's wire values - 0 = MAP_TOWN, the same
    // default the server assigns a player on their first ever state update,
    // so the common (non-portal) case needs no extra message to agree on it.
    uint32_t mCurrentMapId{0};

    std::optional<InitialSpawnState> mInitialSpawnState;
    std::string mLocalUsername;

    // One whisper window per remote player, keyed by their id - created the
    // first time either side sends/receives a message with them, but only
    // shown (Panel::isOpen) once the player actually opens it (right-click
    // -> Chat, or clicking its unread indicator - see
    // renderUnreadChatIndicators). An incoming message while it's closed
    // just queues into its history and flags it unread instead of forcing
    // the window open.
    std::unordered_map<uint64_t, std::shared_ptr<ChatPanel>> mChatPanels;

    void onPlayerJoined(const WorldController::PlayerSnapshot& pSnapshot);
    void onPlayerStateUpdate(const WorldController::PlayerSnapshot& pSnapshot);
    void onPlayerLeft(uint64_t pPlayerId);
    void onMapChanged(const WorldController::MapSnapshot& pSnapshot);
    void onDirectMessageReceived(uint64_t pFromPlayerId, const std::string& pFromUsername, const std::string& pText);

    glm::vec3 getInterpolatedPosition(const RemotePlayerVisual& pVisual) const;
    void rebuildRemotePlayerBoxes();
    void applyMapVisuals();
    void sendLocalPlayerState();

    // Draws pName as screen-space text above pWorldPosition, using ImGui's
    // foreground draw list - reuses the existing GUI stack instead of
    // building a 3D text-mesh/billboard rendering system.
    void renderNameplate(const glm::vec3& pWorldPosition, const std::string& pName);

    // Converts an ImGui window-local pixel coordinate into the global
    // coordinate space ImGui's own draw calls expect - GuiLayer enables
    // ImGuiConfigFlags_ViewportsEnable, under which that global space is the
    // desktop's, not this window's, so the window's own on-screen position
    // (ImGui::GetMainViewport()->Pos) has to be added back in. Shared by the
    // nameplate draw-list text and the right-click context menu's position.
    ImVec2 toGlobalImGuiPos(const ImVec2& pWindowLocalPos) const;

    // Ray-casts pWindowLocalPixel into the world (unprojecting through the
    // camera's inverse view-projection) and tests it against each remote
    // player's axis-aligned box. Returns the closest hit, if any.
    std::optional<TargetInfo> pickTargetAt(const glm::vec2& pWindowLocalPixel) const;

    // Returns the existing ChatPanel for pTargetId, or creates one (added to
    // GuiLayer but closed/hidden - Panel::isOpen = false) if this is the
    // first message either direction with them.
    std::shared_ptr<ChatPanel> ensureChatPanel(uint64_t pTargetId, const std::string& pTargetUsername);

    // Explicitly opens (or brings to front, if already open) a whisper
    // window with pTargetId - only called from user-initiated actions
    // (right-click -> Chat, clicking an unread indicator), never from an
    // incoming-message push itself.
    void openChatWith(uint64_t pTargetId, const std::string& pTargetUsername);

    // Small always-visible overlay, pinned to the right edge of the screen,
    // listing every closed chat with unread messages as a blinking,
    // clickable entry - clicking one opens that conversation.
    void renderUnreadChatIndicators();
};

}

#endif
