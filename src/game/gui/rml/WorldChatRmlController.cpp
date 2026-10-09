#include "WorldChatRmlController.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/DataStructHandle.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Input.h>

#include "RmlUiLayer.h"

#include "../../Engine.h"

using namespace lakot;

WorldChatRmlController::WorldChatRmlController(RmlUiLayer& pRmlUiLayer)
    : mRmlUiLayer(pRmlUiLayer)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("world_chat");

    // Same guard as ChatRmlController::ChatLine, and for the same reason:
    // RmlUi's struct/array type registry is shared per Rml::Context, not
    // per-model. This controller is only ever constructed once per
    // WorldScene, but WorldScene itself can be recreated (e.g. a fresh
    // login after returning to the login screen) within the SAME long-lived
    // context - without this guard, the second WorldScene's
    // WorldChatRmlController would hit "Struct type already declared".
    static bool sChatLineTypeRegistered = false;

    if (!sChatLineTypeRegistered)
    {
        Rml::StructHandle<ChatLine> tLineHandle = tConstructor.RegisterStruct<ChatLine>();
        tLineHandle.RegisterMember("is_local", &ChatLine::isLocal);
        tLineHandle.RegisterMember("author", &ChatLine::author);
        tLineHandle.RegisterMember("text", &ChatLine::text);
        tLineHandle.RegisterMember("is_faded", &ChatLine::isFaded);
        tLineHandle.RegisterMember("is_global", &ChatLine::isGlobal);
        tLineHandle.RegisterMember("is_failed", &ChatLine::isFailed);
        tConstructor.RegisterArray<std::vector<ChatLine>>();

        sChatLineTypeRegistered = true;
    }

    tConstructor.Bind("history", &mHistory);
    tConstructor.Bind("input_text", &mInputText);
    tConstructor.Bind("is_open", &mIsOpen);
    tConstructor.Bind("is_global_mode", &mIsGlobalMode);
    tConstructor.Bind("scope_label", &mScopeButtonLabel);
    tConstructor.Bind("status_text", &mStatusText);
    tConstructor.Bind("has_status", &mHasStatusMessage);

    tConstructor.BindEventCallback("send", &WorldChatRmlController::onSendClicked, this);
    tConstructor.BindEventCallback("input_keydown", &WorldChatRmlController::onInputKeyDown, this);
    tConstructor.BindEventCallback("toggle_scope", &WorldChatRmlController::onScopeToggleClicked, this);

    mModelHandle = tConstructor.GetModelHandle();

    mDocument = mRmlUiLayer.loadDocument("ui/worldchat.rml");

    if (mDocument)
    {
        mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
    }
}

WorldChatRmlController::~WorldChatRmlController()
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        if (mDocument)
        {
            tContext->UnloadDocument(mDocument);
        }

        tContext->RemoveDataModel("world_chat");
    }
}

void WorldChatRmlController::appendLine(const std::string& pAuthorUsername, const std::string& pText, bool pIsLocal, bool pIsGlobal)
{
    ChatLine tLine;
    tLine.isLocal = pIsLocal;
    tLine.author = pAuthorUsername;
    tLine.text = pText;
    tLine.isGlobal = pIsGlobal;

    mHistory.push_back(std::move(tLine));
    mModelHandle.DirtyVariable("history");
    scrollHistoryToBottom();
}

void WorldChatRmlController::clearHistory()
{
    mHistory.clear();
    mModelHandle.DirtyVariable("history");
}

void WorldChatRmlController::setLocalMessageSentCallback(LocalMessageSentCallback pCallback)
{
    mLocalMessageSentCallback = pCallback;
}

void WorldChatRmlController::onSendClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    doSend();
}

void WorldChatRmlController::onScopeToggleClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    mIsGlobalMode = !mIsGlobalMode;
    mScopeButtonLabel = mIsGlobalMode ? "World" : "Within Range";

    mModelHandle.DirtyVariable("is_global_mode");
    mModelHandle.DirtyVariable("scope_label");

    // Re-focus the input so toggling scope mid-typing doesn't cost the
    // player their place - only meaningful while the panel is actually
    // open (the button itself is hidden otherwise, so this is mostly
    // defensive).
    if (mIsOpen && mDocument)
    {
        Rml::Element* tInput = mDocument->GetElementById("chat-input");
        if (tInput)
        {
            tInput->Focus();
        }
    }
}

void WorldChatRmlController::onInputKeyDown(Rml::DataModelHandle /*pHandle*/, Rml::Event& pEvent, const Rml::VariantList& /*pArguments*/)
{
    auto tKeyIdentifier = static_cast<Rml::Input::KeyIdentifier>(pEvent.GetParameter<int>("key_identifier", 0));

    if (tKeyIdentifier == Rml::Input::KI_RETURN || tKeyIdentifier == Rml::Input::KI_NUMPADENTER)
    {
        // Enter with something typed sends and stays open (ready for the
        // next line); Enter on an empty input is the "close" gesture - see
        // class comment for why this is the only place a bare Enter can
        // reach while the panel is open.
        if (mInputText.empty())
        {
            close();
        }
        else
        {
            doSend();
        }
    }
    else if (tKeyIdentifier == Rml::Input::KI_ESCAPE)
    {
        close();
    }
    else
    {
        return;
    }

    // Without this, RmlUi's OWN built-in text-input widget also reacts to
    // this same "keydown" (its Enter handling dispatches a "change" event
    // that re-syncs the model FROM the widget's still-unchanged internal
    // buffer, silently undoing whatever doSend()/close() just did to
    // mInputText - e.g. clearing it here, then having it come right back).
    // Confirmed in scratchpad/rmlrepro: plain StopPropagation() does NOT
    // stop this (it only blocks bubbling to ancestor elements, not other
    // listeners on the SAME element - verified against RmlUi 6.1's
    // EventDispatcher::DispatchEvent), only StopImmediatePropagation() does.
    pEvent.StopImmediatePropagation();
}

void WorldChatRmlController::open()
{
    if (mIsOpen)
    {
        return;
    }

    mIsOpen = true;

    for (ChatLine& tLine : mHistory)
    {
        tLine.isFaded = false;
    }

    mModelHandle.DirtyVariable("is_open");
    mModelHandle.DirtyVariable("history");

    if (!mDocument)
    {
        return;
    }

    // data-if doesn't remove the input row from the tree, it only toggles a
    // display:none style property (confirmed against RmlUi 6.1's own
    // DataViewIf::Update source) - but that toggle still needs a real
    // Update() to actually run before Focus() below, same reasoning as
    // scrollHistoryToBottom()'s forced Update(), or Focus() would be called
    // on an element that's still display:none from the previous frame.
    Rml::Context* tContext = mRmlUiLayer.getContext();
    if (tContext)
    {
        tContext->Update();
    }

    Rml::Element* tInput = mDocument->GetElementById("chat-input");
    if (tInput)
    {
        tInput->Focus();
    }
}

void WorldChatRmlController::close()
{
    if (!mIsOpen)
    {
        return;
    }

    mIsOpen = false;

    // The rejection notice belongs to the attempt that caused it - closing
    // the panel ends that moment, so it should not still be sitting there the
    // next time the panel is opened.
    mHasStatusMessage = false;
    mStatusText.clear();

    // A fresh kMessageVisibleSeconds for every line still on screen, rather
    // than instantly fading anything that was already older than that while
    // fading was paused (see class comment) - "you just read these, they
    // stay up a bit longer" rather than an abrupt disappearance on close.
    for (ChatLine& tLine : mHistory)
    {
        tLine.isFaded = false;
        tLine.ageSeconds = 0.0;
    }

    mModelHandle.DirtyVariable("is_open");
    mModelHandle.DirtyVariable("history");

    if (mDocument)
    {
        Rml::Element* tInput = mDocument->GetElementById("chat-input");
        if (tInput)
        {
            tInput->Blur();
        }
    }
}

void WorldChatRmlController::update(double pDeltaTime)
{
    // Fading is paused entirely while the panel is open - see class
    // comment and open()/close().
    if (mIsOpen)
    {
        return;
    }

    bool tAnyChanged = false;

    for (ChatLine& tLine : mHistory)
    {
        if (tLine.isFaded)
        {
            continue;
        }

        tLine.ageSeconds += pDeltaTime;

        if (tLine.ageSeconds >= kMessageVisibleSeconds)
        {
            tLine.isFaded = true;
            tAnyChanged = true;
        }
    }

    if (tAnyChanged)
    {
        mModelHandle.DirtyVariable("history");
    }
}

void WorldChatRmlController::doSend()
{
    if (mInputText.empty())
    {
        return;
    }

    std::string tRawText(mInputText);

    // Two ways to send this one as global, combined with OR: the scope
    // button (mIsGlobalMode, a persistent default) or an explicit leading
    // "!" on this particular message, which always wins regardless of the
    // button - the "!" is stripped either way, it's never shown as content.
    bool tTypedBang = !tRawText.empty() && tRawText.front() == '!';
    std::string tDisplayText = tTypedBang ? tRawText.substr(1) : tRawText;
    bool tIsGlobal = mIsGlobalMode || tTypedBang;

    // A bare "!" strips down to nothing. The empty check at the top of this
    // function only sees the RAW input, so this used to send an empty message
    // that the server then rejected as invalid - an error for what is really
    // just an unfinished line. Clear it and wait for the rest instead.
    if (tDisplayText.empty())
    {
        mInputText.clear();
        dirtyAll();
        return;
    }

    // Clean text (marker already stripped) plus an explicit flag, decided
    // entirely here - the server just trusts it now, no text-sniffing on
    // its end (see ChatController::handleChatMessageRequest server-side).
    Engine::getInstance().getNetworkManager().getChatController().sendChatMessage(tDisplayText, tIsGlobal);

    // A fresh send clears any leftover rejection text from a previous attempt.
    mHasStatusMessage = false;
    mStatusText.clear();

    // Appended optimistically - the server only broadcasts to everyone ELSE
    // (map-wide or server-wide, depending on tIsGlobal), it doesn't echo the
    // text back to the sender, same reason as ChatRmlController::doSend()'s
    // local "You" append. Remembered by index so onSendResult() can mark THIS
    // specific line red if the server turns out to have refused it.
    appendLine("You", tDisplayText, true, tIsGlobal);
    mLastSentLineIndex = mHistory.size() - 1;
    mHasPendingSend = true;
    mInputText.clear();

    dirtyAll();

    if (mLocalMessageSentCallback)
    {
        mLocalMessageSentCallback(tIsGlobal, tDisplayText);
    }
}

void WorldChatRmlController::onSendResult(bool pIsSuccess, const std::string& pMessage)
{
    if (!mHasPendingSend)
    {
        return; // no send of ours is waiting on an answer
    }

    mHasPendingSend = false;

    if (pIsSuccess)
    {
        return; // the line is already in the history and went out fine
    }

    if (mLastSentLineIndex < mHistory.size())
    {
        mHistory[mLastSentLineIndex].isFailed = true;
    }

    mStatusText = pMessage;
    mHasStatusMessage = true;

    // The panel may well be closed by now (a message can be sent and the
    // panel closed before the answer lands) - reopening it is what makes the
    // rejection actually visible rather than a red line that fades out
    // unread.
    open();

    dirtyAll();
}

void WorldChatRmlController::scrollHistoryToBottom()
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

void WorldChatRmlController::dirtyAll()
{
    mModelHandle.DirtyAllVariables();
}
