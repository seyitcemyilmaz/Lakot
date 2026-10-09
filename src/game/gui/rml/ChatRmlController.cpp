#include "ChatRmlController.h"

#include <SDL3/SDL.h>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/DataStructHandle.h>
#include <RmlUi/Core/Input.h>

#include "RmlUiLayer.h"

#include "../../Engine.h"

using namespace lakot;

ChatRmlController::ChatRmlController(RmlUiLayer& pRmlUiLayer, uint64_t pTargetPlayerId, const std::string& pTargetUsername, float pInitialLeft, float pInitialTop)
    : mRmlUiLayer(pRmlUiLayer)
    , mTargetPlayerId(pTargetPlayerId)
    , mDataModelName("chat_" + std::to_string(pTargetPlayerId))
    , mTargetUsername(pTargetUsername)
    , mIsTargetConfirmed(!pTargetUsername.empty())
{
    mFullWindowLeft = std::to_string(pInitialLeft) + "dp";
    mFullWindowTop = std::to_string(pInitialTop) + "dp";

    Rml::Context* tContext = mRmlUiLayer.getContext();

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel(mDataModelName);

    // RmlUi's struct/array type registry is shared per Rml::Context, not
    // per-model - registering the same C++ type (ChatLine) again for every
    // new conversation's model hits "Struct type already declared", which is
    // only a log warning in Release but an RMLUI_ASSERT dialog in Debug
    // (confirmed - this used to crash/hang opening a second whisper window).
    // Registering it once is enough; every later model's Bind() still finds
    // it via the context's shared registry.
    static bool sChatLineTypeRegistered = false;

    if (!sChatLineTypeRegistered)
    {
        Rml::StructHandle<ChatLine> tLineHandle = tConstructor.RegisterStruct<ChatLine>();
        tLineHandle.RegisterMember("is_local", &ChatLine::isLocal);
        tLineHandle.RegisterMember("author", &ChatLine::author);
        tLineHandle.RegisterMember("text", &ChatLine::text);
        tLineHandle.RegisterMember("is_failed", &ChatLine::isFailed);
        tConstructor.RegisterArray<std::vector<ChatLine>>();

        sChatLineTypeRegistered = true;
    }

    tConstructor.Bind("target_username", &mTargetUsername);
    tConstructor.Bind("history", &mHistory);
    tConstructor.Bind("input_text", &mInputText);
    tConstructor.Bind("is_minimized", &mIsMinimized);
    tConstructor.Bind("has_unread", &mHasUnread);
    tConstructor.Bind("is_target_confirmed", &mIsTargetConfirmed);
    tConstructor.Bind("status_text", &mStatusText);
    tConstructor.Bind("has_status", &mHasStatusMessage);

    tConstructor.BindEventCallback("send", &ChatRmlController::onSendClicked, this);
    tConstructor.BindEventCallback("minimize", &ChatRmlController::onMinimizeClicked, this);
    tConstructor.BindEventCallback("close", &ChatRmlController::onCloseClicked, this);
    tConstructor.BindEventCallback("drag_start", &ChatRmlController::onDragStart, this);
    tConstructor.BindEventCallback("input_keydown", &ChatRmlController::onInputKeyDown, this);
    tConstructor.BindEventCallback("confirm_target", &ChatRmlController::onConfirmTargetClicked, this);
    tConstructor.BindEventCallback("name_keydown", &ChatRmlController::onNameKeyDown, this);

    mModelHandle = tConstructor.GetModelHandle();

    mDocument = mRmlUiLayer.loadDocumentFromTemplate("ui/chat.rml", "__CHAT_MODEL__", mDataModelName);

    if (mDocument)
    {
        mDocument->SetProperty("left", mFullWindowLeft);
        mDocument->SetProperty("top", mFullWindowTop);
        // Left hidden - LoadDocument(FromMemory) never shows a document on
        // its own. Only show() (right-click -> Chat) or the first incoming
        // message (see appendIncoming) reveals it, matching ChatPanel's
        // original isOpen=false-until-opened contract.
    }
}

ChatRmlController::~ChatRmlController()
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        if (mDocument)
        {
            tContext->UnloadDocument(mDocument);
        }

        // mDataModelName, not a name rebuilt from mTargetPlayerId - that id
        // can have been re-pointed since construction (setTargetPlayerId).
        tContext->RemoveDataModel(mDataModelName);
    }
}

uint64_t ChatRmlController::getTargetPlayerId() const
{
    return mTargetPlayerId;
}

void ChatRmlController::setTargetPlayerId(uint64_t pTargetPlayerId)
{
    mTargetPlayerId = pTargetPlayerId;
}

const std::string& ChatRmlController::getTargetUsername() const
{
    return mTargetUsername;
}

bool ChatRmlController::isTargetConfirmed() const
{
    return mIsTargetConfirmed;
}

void ChatRmlController::openForNewTarget()
{
    mIsMinimized = false;
    mHasUnread = false;
    dirtyAll();

    if (!mDocument)
    {
        return;
    }

    mDocument->Show();
    mDocument->PullToFront();

    // Same reasoning as WorldChatRmlController::open() - force a real
    // Update() before Focus() rather than assume Show() alone already put
    // things in a focusable state this
    // exact frame.
    Rml::Context* tContext = mRmlUiLayer.getContext();
    if (tContext)
    {
        tContext->Update();
    }

    Rml::Element* tNameInput = mDocument->GetElementById("chat-name-input");
    if (tNameInput)
    {
        tNameInput->Focus();
    }
}

void ChatRmlController::appendIncoming(const std::string& pAuthorUsername, const std::string& pText)
{
    bool tHadNeverBeenShown = mDocument && !mDocument->IsVisible();

    mHistory.push_back(ChatLine{ false, pAuthorUsername, pText });
    mModelHandle.DirtyVariable("history");
    scrollHistoryToBottom();

    if (!isFullyOpen())
    {
        mHasUnread = true;
    }

    if (tHadNeverBeenShown && mDocument)
    {
        mIsMinimized = true;
        // FocusFlag::None - a whisper arriving shouldn't steal keyboard
        // focus away from whatever the player is doing (moving, typing in
        // another chat window).
        mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
    }

    dirtyAll();
}

bool ChatRmlController::hasUnread() const
{
    return mHasUnread;
}

bool ChatRmlController::isMinimized() const
{
    return mIsMinimized;
}

bool ChatRmlController::isFullyOpen() const
{
    return mDocument && mDocument->IsVisible() && !mIsMinimized;
}

void ChatRmlController::minimize()
{
    doMinimize();
}

void ChatRmlController::setIconTopPosition(float pTop)
{
    if (!mDocument || !mIsMinimized)
    {
        return;
    }

    mDocument->SetProperty("top", std::to_string(pTop) + "dp");
}

void ChatRmlController::show()
{
    mIsMinimized = false;
    mHasUnread = false;
    dirtyAll();

    if (mDocument)
    {
        mDocument->Show();
        mDocument->PullToFront();
    }
}

void ChatRmlController::update()
{
    if (!mIsDragging || !mDocument)
    {
        return;
    }

    Uint32 tButtons = SDL_GetMouseState(nullptr, nullptr);
    if (!(tButtons & SDL_BUTTON_LMASK))
    {
        mIsDragging = false;
        return;
    }

    float tMouseX = 0.0f;
    float tMouseY = 0.0f;
    SDL_GetMouseState(&tMouseX, &tMouseY);

    Rml::Vector2f tClamped = mRmlUiLayer.clampDocumentToScreen(mDocument, mDocument,
        Rml::Vector2f(mDragStartDocLeft + (tMouseX - mDragStartMouseX), mDragStartDocTop + (tMouseY - mDragStartMouseY)));

    float tNewLeft = tClamped.x;
    float tNewTop = tClamped.y;

    // Dragging only happens in full-window mode (the titlebar that starts it
    // doesn't exist while minimized), so this is always the position to
    // return to later - keep it current.
    mFullWindowLeft = std::to_string(tNewLeft) + "px";
    mFullWindowTop = std::to_string(tNewTop) + "px";

    mDocument->SetProperty("left", mFullWindowLeft);
    mDocument->SetProperty("top", mFullWindowTop);
}

void ChatRmlController::onSendClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    doSend();
}

void ChatRmlController::onInputKeyDown(Rml::DataModelHandle /*pHandle*/, Rml::Event& pEvent, const Rml::VariantList& /*pArguments*/)
{
    auto tKeyIdentifier = static_cast<Rml::Input::KeyIdentifier>(pEvent.GetParameter<int>("key_identifier", 0));

    if (tKeyIdentifier == Rml::Input::KI_RETURN || tKeyIdentifier == Rml::Input::KI_NUMPADENTER)
    {
        doSend();
        // NOT calling StopImmediatePropagation() here - this send path has
        // the same latent "input doesn't fully clear" issue flagged earlier
        // this session (RmlUi's own text-input default action re-syncing
        // the model right after doSend()'s clear) but that fix is still
        // waiting on explicit approval; left as-is on purpose rather than
        // folding it in as a side effect of the unrelated Escape change below.
    }
    else if (tKeyIdentifier == Rml::Input::KI_ESCAPE)
    {
        doMinimize();

        // Not strictly load-bearing here (only Return has a competing
        // default action to worry about), but consistent with every other
        // Escape-handling keydown callback in this codebase.
        pEvent.StopImmediatePropagation();
    }
}

void ChatRmlController::doSend()
{
    if (mInputText.empty())
    {
        return;
    }

    std::string tText(mInputText);

    mPendingRequestId = Engine::getInstance().getNetworkManager().getChatController().sendDirectMessage(mTargetUsername, tText);

    // A fresh send clears any leftover failure text from a previous
    // attempt - otherwise a stale "User is not active." would keep sitting
    // there under a message that actually went through fine this time.
    mHasStatusMessage = false;
    mStatusText.clear();

    // Appended optimistically - the server only acks success/failure, it
    // doesn't echo the text back, so the sender's own window would
    // otherwise never show what they just sent. Remembered by index so
    // onSendResult() can mark THIS specific line red if it turns out to
    // have failed - see mLastSentLineIndex's own comment.
    mHistory.push_back(ChatLine{ true, "You", tText });
    mLastSentLineIndex = mHistory.size() - 1;
    mHasPendingSentLine = true;
    mInputText.clear();

    if (mPendingRequestId == 0)
    {
        // Nothing left the client (no connection) - there will be no result
        // to wait for, so mark the optimistic line failed right here instead
        // of leaving it looking sent forever.
        mHistory[mLastSentLineIndex].isFailed = true;
        mHasPendingSentLine = false;

        mStatusText = "No connection to server!";
        mHasStatusMessage = true;
    }

    dirtyAll();
    scrollHistoryToBottom();
}

void ChatRmlController::onSendResult(uint64_t pRequestId, bool pIsSuccess, const std::string& pMessage)
{
    if (mPendingRequestId == 0 || pRequestId != mPendingRequestId)
    {
        return; // some OTHER window's send (or an uncorrelated result) - ignore
    }

    mPendingRequestId = 0;

    if (pIsSuccess)
    {
        mHasPendingSentLine = false;
        return; // nothing to show - the sent message is already in mHistory
    }

    if (mHasPendingSentLine && mLastSentLineIndex < mHistory.size())
    {
        mHistory[mLastSentLineIndex].isFailed = true;
    }
    mHasPendingSentLine = false;

    mStatusText = pMessage;
    mHasStatusMessage = true;
    dirtyAll();
}

void ChatRmlController::onConfirmTargetClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    doConfirmTarget();
}

void ChatRmlController::onNameKeyDown(Rml::DataModelHandle /*pHandle*/, Rml::Event& pEvent, const Rml::VariantList& /*pArguments*/)
{
    auto tKeyIdentifier = static_cast<Rml::Input::KeyIdentifier>(pEvent.GetParameter<int>("key_identifier", 0));

    if (tKeyIdentifier == Rml::Input::KI_RETURN || tKeyIdentifier == Rml::Input::KI_NUMPADENTER)
    {
        doConfirmTarget();
    }
    else if (tKeyIdentifier == Rml::Input::KI_ESCAPE)
    {
        doMinimize();
    }
    else
    {
        return;
    }

    // Same fix as WorldChatRmlController::onInputKeyDown - RmlUi's own
    // built-in text-input default action re-syncs the model from its
    // internal buffer right after our own handling, undoing it, UNLESS
    // propagation is stopped immediately (plain StopPropagation() doesn't
    // reach far enough - it only blocks bubbling to ancestors, not other
    // listeners on this same element).
    pEvent.StopImmediatePropagation();
}

void ChatRmlController::doConfirmTarget()
{
    if (mTargetUsername.empty() || mIsTargetConfirmed)
    {
        return;
    }

    mIsTargetConfirmed = true;
    dirtyAll();

    Rml::Context* tContext = mRmlUiLayer.getContext();
    if (tContext)
    {
        tContext->Update(); // same data-if/display:none timing reasoning as elsewhere
    }

    if (mDocument)
    {
        Rml::Element* tMessageInput = mDocument->GetElementById("chat-message-input");
        if (tMessageInput)
        {
            tMessageInput->Focus();
        }
    }
}

void ChatRmlController::doMinimize()
{
    if (mIsMinimized)
    {
        return;
    }

    mIsMinimized = true;
    dirtyAll();
}

void ChatRmlController::onMinimizeClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    mIsMinimized = !mIsMinimized;

    // Restoring the window (either from the titlebar's own minimize button,
    // or - since this is the same "minimize" event - by clicking the
    // right-edge mail icon while minimized) counts as reading it, same as
    // show(). Without this, has_unread stayed true forever once set, so
    // re-minimizing an already-read conversation kept blinking.
    if (!mIsMinimized)
    {
        mHasUnread = false;
    }

    dirtyAll();
}

void ChatRmlController::onCloseClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    if (mDocument)
    {
        mDocument->Hide();
    }
}

void ChatRmlController::onDragStart(Rml::DataModelHandle /*pHandle*/, Rml::Event& pEvent, const Rml::VariantList& /*pArguments*/)
{
    int tButton = pEvent.GetParameter<int>("button", -1);

    if (tButton != 0 || !mDocument)
    {
        return; // left button only
    }

    mIsDragging = true;
    SDL_GetMouseState(&mDragStartMouseX, &mDragStartMouseY);
    mDragStartDocLeft = mDocument->GetAbsoluteLeft();
    mDragStartDocTop = mDocument->GetAbsoluteTop();
}

void ChatRmlController::scrollHistoryToBottom()
{
    if (!mDocument)
    {
        return;
    }

    Rml::Context* tContext = mRmlUiLayer.getContext();
    if (tContext)
    {
        tContext->Update(); // force layout so GetScrollHeight() is current
    }

    Rml::Element* tHistory = mDocument->GetElementById("history");
    if (tHistory)
    {
        tHistory->SetScrollTop(tHistory->GetScrollHeight());
    }
}

void ChatRmlController::dirtyAll()
{
    mModelHandle.DirtyAllVariables();

    // "is-minimized" used to be a plain data-class binding straight on
    // <body>, but body can no longer carry the data model (see chat.rml) -
    // so it's applied directly here instead, at the same point every other
    // piece of state gets pushed to the UI.
    if (mDocument)
    {
        mDocument->SetClass("is-minimized", mIsMinimized);

        // Full window vs. right-edge icon each own a different horizontal
        // scheme: full window is C++-positioned "left", icon mode is a fixed
        // CSS "right" that "left" would otherwise fight (see
        // .chat-window.is-minimized in theme.rcss). Re-applying the correct
        // one every time, rather than only on the instant it toggles, means
        // there's no separate "did this just change" edge case to track.
        // Vertical ("top") is left alone here while minimized - WorldScene
        // assigns that every frame via setIconTopPosition() based on the
        // current stack of minimized conversations.
        if (mIsMinimized)
        {
            mDocument->SetProperty("left", "auto");
        }
        else
        {
            mDocument->SetProperty("left", mFullWindowLeft);
            mDocument->SetProperty("top", mFullWindowTop);
        }
    }
}
