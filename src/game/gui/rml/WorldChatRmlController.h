#ifndef LAKOT_WORLDCHATRMLCONTROLLER_H
#define LAKOT_WORLDCHATRMLCONTROLLER_H

#include <functional>
#include <string>
#include <vector>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>

namespace lakot
{

class RmlUiLayer;

// RmlUi panel for the map-wide "say" channel - unlike ChatRmlController
// (one instance per whisper conversation, created on demand, minimizable to
// a right-edge icon), this is a single instance WorldScene owns for its
// whole lifetime. Loads its own worldchat.rml document with a single named
// data model ("world_chat") - no per-instance unique naming needed since
// only one of these ever exists per Rml::Context, unlike chat.rml's
// templating.
//
// Floating "ambient overlay" behavior (Lineage/WoW-style), not a normal
// window: by default it's just unbordered text sitting above the skill bar
// (see .world-chat-window in theme.rcss) - each line fades out 5s after
// being posted (see update()). Pressing Enter while nothing has RmlUi focus
// (WorldScene::handleEvent) calls open(): reveals the full history, pauses
// fading, shows+focuses the input row. From there, Enter with text sends
// (doSend(), panel stays open for the next message); Enter with an empty
// input, or Escape, calls close() - both reach onInputKeyDown() below since
// RmlUiLayer::handleEvent forces every key event to the focused input while
// this document has focus (see its own comment for why), which is also
// exactly what keeps a bare Enter from re-opening this over top of itself
// while it's already open.
//
// Scope (world-wide vs nearby-only) is chosen two ways, combined with OR:
// the "World"/"Within Range" toggle button to the left of the input
// (mIsGlobalMode, persists until clicked again) sets the default for
// whatever's typed next, and typing a leading "!" forces that ONE message
// global regardless of the button - see doSend().
class WorldChatRmlController
{
public:
    // Fired from doSend() right after the optimistic local append, so
    // WorldScene can also start the sender's own nameplate speech-bubble for
    // a nearby (non-"!") message - the server never echoes a message back to
    // its own sender (see doSend()'s comment), so this is the only way
    // WorldScene learns "I just said this" at all; incoming messages from
    // OTHER players reach it through ChatController::ChatMessageReceivedCallback
    // instead.
    using LocalMessageSentCallback = std::function<void(bool pIsGlobal, const std::string& pText)>;

    explicit WorldChatRmlController(RmlUiLayer& pRmlUiLayer);
    ~WorldChatRmlController();

    WorldChatRmlController(const WorldChatRmlController&) = delete;
    WorldChatRmlController& operator=(const WorldChatRmlController&) = delete;

    // pIsLocal marks whether this line came from the local player (sent via
    // doSend()'s optimistic append) or someone else on the map (pushed by
    // ChatController::setChatMessageReceivedCallback) - same
    // msg-local/msg-remote coloring convention as whisper's ChatLine (that's
    // about WHO sent it). pIsGlobal is a separate, orthogonal distinction -
    // WHAT KIND of message it was ("!" server-wide vs nearby-only) - driving
    // worldchat.rml's .msg-global/.msg-nearby text coloring.
    void appendLine(const std::string& pAuthorUsername, const std::string& pText, bool pIsLocal, bool pIsGlobal);

    // Called from WorldScene::onMapChanged() - a new map's "say" channel
    // starts with a clean slate, same reasoning as clearing the remote entities.
    void clearHistory();

    void setLocalMessageSentCallback(LocalMessageSentCallback pCallback);

    // Wired to ChatController::ChatMessageResultCallback by WorldScene. On
    // failure the optimistically-appended line is marked failed (red) and
    // pMessage is shown under the input row - the same treatment whisper
    // windows already give a rejected send, rather than a console log the
    // player never sees.
    void onSendResult(bool pIsSuccess, const std::string& pMessage);

    // Called from WorldScene::handleEvent on a bare Enter press (nothing
    // else has RmlUi focus at that point, so this can only mean the panel is
    // currently closed - see class comment). Reveals the full history
    // (clears every line's faded state) and pauses fading (see update())
    // until close() runs, then focuses the input.
    void open();

    // Called from onInputKeyDown() - Enter on an empty input, or Escape.
    // Hides the input row again and gives every currently-visible line a
    // fresh kMessageVisibleSeconds before it starts fading once more (rather
    // than instantly fading anything that was already older than that while
    // fading was paused).
    void close();

    // Call once per frame (WorldScene::update()) - ages every line while the
    // panel is closed and marks any that crossed kMessageVisibleSeconds as
    // faded (.chat-line.faded's own `transition` in theme.rcss does the
    // actual fade-out smoothly, same as the skill bar's flash - no
    // per-frame opacity math needed here). No-op entirely while open()
    // hasn't been matched by a close() yet.
    void update(double pDeltaTime);

private:
    static constexpr double kMessageVisibleSeconds = 5.0;

    struct ChatLine
    {
        bool isLocal;
        Rml::String author;
        Rml::String text;
        // Server-wide message (true) vs nearby-only (false) - see
        // appendLine()'s comment. Drives .msg-global/.msg-nearby in
        // worldchat.rml/theme.rcss.
        bool isGlobal{false};
        // Both below are world-chat-only fade-out state - isFaded is bound
        // to the model (data-class-faded in worldchat.rml), ageSeconds is
        // plain internal bookkeeping RmlUi never sees (registering a struct
        // only reflects the members explicitly given to RegisterMember, so
        // an extra unregistered field like this is fine).
        bool isFaded{false};
        double ageSeconds{0.0};

        // Set after the fact by onSendResult() when the server refuses this
        // line (the world-message cooldown, most often). Drives .msg-failed,
        // the same red whisper already uses for a rejected send.
        bool isFailed{false};
    };

    RmlUiLayer& mRmlUiLayer;

    Rml::ElementDocument* mDocument{nullptr};
    Rml::DataModelHandle mModelHandle;

    std::vector<ChatLine> mHistory;
    Rml::String mInputText;
    bool mIsOpen{false};

    // The scope toggle button's state/label - see class comment. Persists
    // across sends (a deliberate mode, not a one-shot flag) until clicked
    // again.
    bool mIsGlobalMode{false};
    Rml::String mScopeButtonLabel{"Within Range"};

    // Rejected-send feedback, shown under the input row. mLastSentLineIndex
    // remembers which optimistically-appended line a pending result belongs
    // to, so THAT line turns red rather than the newest one; mHasPendingSend
    // guards it because 0 is a valid index and cannot double as "none".
    Rml::String mStatusText;
    bool mHasStatusMessage{false};
    size_t mLastSentLineIndex{0};
    bool mHasPendingSend{false};

    LocalMessageSentCallback mLocalMessageSentCallback;

    void onSendClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);

    // data-event-click on the scope button - flips mIsGlobalMode and
    // refreshes mScopeButtonLabel to match.
    void onScopeToggleClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);

    // Same fix as ChatRmlController/LoginRmlController - RmlUi's SDL backend
    // turns Enter into a literal '\n' text-input character rather than a
    // submit action, and the input isn't wrapped in a <form>. Also where
    // Escape, and an empty-input Enter, route to close() - see class
    // comment for why this is the only place that can ever see those two.
    void onInputKeyDown(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);

    void doSend();

    void scrollHistoryToBottom();
    void dirtyAll();
};

}

#endif
