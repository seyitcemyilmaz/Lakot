#ifndef LAKOT_CHATRMLCONTROLLER_H
#define LAKOT_CHATRMLCONTROLLER_H

#include <cstdint>
#include <string>
#include <vector>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>

namespace lakot
{

class RmlUiLayer;

// RmlUi replacement for the ImGui ChatPanel - one instance per whisper
// conversation (owned by WorldScene, keyed by target player id), each
// loading its own instance of the shared chat.rml template. See
// RmlUiLayer::loadDocumentFromTemplate for why each instance needs its own
// uniquely-named data model.
//
// A window can also start with NO known target: WorldScene::
// openBlankWhisperWindow() (Shift+Enter) constructs one with an empty
// pTargetUsername, keyed by a synthetic id (real ids never reach that
// range - see its own comment) rather than a right-click's already-known
// player id. Passing an empty username is exactly what puts a window in
// this "unconfirmed" state (mIsTargetConfirmed) - chat.rml then shows an
// editable name field (with a confirm "OK" button/Enter) in the titlebar
// instead of the plain read-only name, and hides the message row entirely
// until doConfirmTarget() runs, per the user's own requested flow (type a
// name, confirm, then the message box becomes usable). The window is
// otherwise a completely normal ChatRmlController once confirmed - same
// minimize/close/drag, same sendDirectMessage()-by-username send path
// (which never needed a numeric id to begin with).
class ChatRmlController
{
public:
    ChatRmlController(RmlUiLayer& pRmlUiLayer, uint64_t pTargetPlayerId, const std::string& pTargetUsername, float pInitialLeft, float pInitialTop);
    ~ChatRmlController();

    ChatRmlController(const ChatRmlController&) = delete;
    ChatRmlController& operator=(const ChatRmlController&) = delete;

    uint64_t getTargetPlayerId() const;
    const std::string& getTargetUsername() const;

    // Re-points a window opened before its target's real player id was known
    // (a blank Shift+Enter window, keyed by a synthetic id) at that real id
    // once the other side replies - see WorldScene::onDirectMessageReceived.
    // Without this, the reply created a SECOND window for the same
    // conversation. Only the map key and this field change; the RmlUi data
    // model keeps the name it was created with (mDataModelName), since a live
    // model cannot be renamed and its name only ever needed to be unique.
    void setTargetPlayerId(uint64_t pTargetPlayerId);

    // True once a target username has been set and confirmed (always true
    // for a window constructed with a non-empty pTargetUsername - right-
    // click or an incoming message already know it; only a blank Shift+Enter
    // window starts false). See class comment.
    bool isTargetConfirmed() const;

    // Called from WorldScene::openBlankWhisperWindow() instead of show() -
    // same effect (un-minimize, clear unread, show/raise) but focuses the
    // editable name field instead, since there's nothing to read/type a
    // message into yet.
    void openForNewTarget();

    // Same contract as the old ChatPanel: appends a line, and only flags
    // unread if the window isn't fully expanded/visible right now. Unlike
    // before, there's no separate top-right aggregator for a conversation
    // that has never been opened at all - the first message instead reveals
    // the window minimized, which renders as the right-edge blinking mail
    // icon (see chat.rml/theme.rcss), so a new whisper is always noticeable
    // without a second UI element the reference photo didn't show.
    void appendIncoming(const std::string& pAuthorUsername, const std::string& pText);

    bool hasUnread() const;

    // Un-minimizes, clears unread, shows/raises the window - called only
    // from user-initiated opens (right-click -> Chat).
    void show();

    bool isMinimized() const;

    // True while shown as a full window (not hidden, not minimized). Lets
    // WorldScene's Escape handling minimize a window that has no focused
    // input to do it itself.
    bool isFullyOpen() const;

    // Same effect as doMinimize() (see its own comment) - a public entry
    // point for WorldScene's Escape fallback above, mirroring show()'s
    // existing public/private split.
    void minimize();

    // While minimized, the window renders as a small right-edge notification
    // icon instead of a full chat window (see chat.rml's is_minimized branch
    // and .chat-window.is-minimized in theme.rcss, which anchors it to the
    // right edge via a fixed `right`, leaving only vertical position to be
    // assigned here). WorldScene calls this every frame for every currently-
    // minimized controller, stacking them top-to-bottom so they don't
    // overlap - a no-op while not minimized.
    void setIconTopPosition(float pTop);

    // Per-frame upkeep - a no-op unless the title bar is currently being
    // dragged (see mIsDragging). Call once per frame from WorldScene::update().
    void update();

    // Called from WorldScene's shared ChatController::DirectMessageResultCallback
    // for EVERY open ChatRmlController - a no-op here unless pRequestId is
    // the id THIS window's own doSend() is waiting on (mPendingRequestId).
    // The correlation is real now: the client stamps RequestHeader.id on the
    // whisper and the server echoes it back as ResponseHeader.reply_to, so
    // two windows sending before either result arrives each get their own
    // answer instead of both consuming whichever landed first.
    // On failure, shows pMessage in the window itself (red status line,
    // chat.rml) instead of only logging it to the console.
    void onSendResult(uint64_t pRequestId, bool pIsSuccess, const std::string& pMessage);

private:
    struct ChatLine
    {
        bool isLocal;
        Rml::String author;
        Rml::String text;
        // Set after the fact by onSendResult() if this particular sent line's
        // DirectMessageRequest came back with an error - see mLastSentLineIndex.
        // Drives .msg-failed (red) in chat.rml/theme.rcss, so a failed send is
        // told apart from a successful one at a glance instead of only a
        // separate status line underneath.
        bool isFailed{false};
    };

    RmlUiLayer& mRmlUiLayer;
    uint64_t mTargetPlayerId;

    // Fixed for this window's whole life, derived from the id it was CREATED
    // with. mTargetPlayerId can change afterwards (setTargetPlayerId), and an
    // already-constructed RmlUi data model cannot be renamed - so the name
    // used to create the model has to be remembered separately, or the
    // destructor's RemoveDataModel() would look up a name that was never
    // registered and leak the model.
    std::string mDataModelName;

    Rml::String mTargetUsername;
    // See class comment - starts false only for a blank Shift+Enter window
    // (constructed with an empty pTargetUsername), true otherwise. Declared
    // (and initialized) right after mTargetUsername - the constructor's
    // initializer list sets this from it, so declaration order matters here.
    bool mIsTargetConfirmed;

    Rml::ElementDocument* mDocument{nullptr};
    Rml::DataModelHandle mModelHandle;

    std::vector<ChatLine> mHistory;
    Rml::String mInputText;
    bool mIsMinimized{false};
    bool mHasUnread{false};

    // Failed-send feedback, shown as a red line under the input row (see
    // chat.rml/theme.rcss) instead of only a console log. mPendingRequestId
    // is the RequestHeader.id of the send this window is currently waiting on
    // (0 = not waiting); onSendResult() only reacts to a result carrying that
    // same id back in ResponseHeader.reply_to.
    uint64_t mPendingRequestId{0};
    Rml::String mStatusText;
    bool mHasStatusMessage{false};

    // Index into mHistory of the line doSend() most recently pushed
    // optimistically - onSendResult() marks THAT line's isFailed if the
    // send didn't go through. mHasPendingSentLine guards against a stray
    // index (0 is a valid index, so it can't double as "none" on its own).
    // Still a single slot: sending twice from the SAME window before the
    // first result arrives attributes both results to the second line. That
    // is a much narrower window than the old cross-window mix-up (a whisper
    // ack comes back in one round trip), so it stays as-is rather than
    // growing a per-line pending map.
    size_t mLastSentLineIndex{0};
    bool mHasPendingSentLine{false};

    // Manual drag-to-move: RmlUi documents have no native window-chrome
    // dragging the way ImGui windows did for free. mousedown on the title
    // bar starts this; update() (polled every frame, not event-driven, so
    // it keeps tracking even if the cursor outruns the moving title bar's
    // own bounds) applies the delta and ends the drag once the left mouse
    // button is no longer held.
    bool mIsDragging{false};
    float mDragStartMouseX{0.0f};
    float mDragStartMouseY{0.0f};
    float mDragStartDocLeft{0.0f};
    float mDragStartDocTop{0.0f};

    // The position to return to when un-minimizing - initialized from the
    // constructor's staggered position, kept in sync by update()'s drag
    // handling (dragging only happens in full-window mode, so any drag is a
    // full-window position by definition). While minimized, the document's
    // actual on-screen position is the right-edge icon slot instead (see
    // setIconTopPosition/dirtyAll), not this.
    Rml::String mFullWindowLeft;
    Rml::String mFullWindowTop;

    void onSendClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onMinimizeClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onCloseClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onDragStart(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onConfirmTargetClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);

    // RmlUi's SDL backend turns Enter into a literal '\n' text-input
    // character rather than a submit action (see RmlUi_Platform_SDL.cpp),
    // and the input isn't wrapped in a <form> - so without this, Enter does
    // nothing at all. Bound to the input's keydown, this just filters for
    // Return/Numpad-Enter and forwards to the same send logic the button
    // uses; Escape minimizes (see doMinimize()).
    void onInputKeyDown(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);

    // Same idea as onInputKeyDown() but for the editable name field - Enter
    // confirms (same as clicking "OK"), Escape minimizes (same as pressing
    // Escape in the message field, or the titlebar's own minimize button).
    void onNameKeyDown(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);

    // Shared by onSendClicked() and onInputKeyDown() (Enter) - the actual
    // "send whatever is in the box" logic, independent of what triggered it.
    void doSend();

    // Shared by onConfirmTargetClicked() and onNameKeyDown() (Enter) - a
    // no-op if the name field is empty (nothing to confirm) or the target is
    // already confirmed. Focuses the message input afterward.
    void doConfirmTarget();

    // Shared by onInputKeyDown()/onNameKeyDown() (Escape, in either field) -
    // a plain minimize (not onMinimizeClicked()'s toggle - Escape can only
    // ever be pressed while NOT minimized, since a minimized window has no
    // focused text input for the key event to reach in the first place).
    void doMinimize();

    void scrollHistoryToBottom();
    void dirtyAll();
};

}

#endif
