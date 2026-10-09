#include "RmlUiLayer.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include <RmlUi/Core.h>

#include "FileManager.h"

using namespace lakot;

void RmlUiLayer::initialize(SDL_Window* pWindow, SDL_GLContext pGLContext)
{
    (void)pGLContext;

    mWindow = pWindow;

    // No-op when RMLUI_GL3_CUSTOM_LOADER is set (src/game/CMakeLists.txt) -
    // GL functions are already loaded via glad in main.cpp before
    // Engine::initialize() runs.
    RmlGL3::Initialize();

    // Constructed here, not as plain members - see the comment on their
    // declaration in RmlUiLayer.h for why (both constructors need a live GL
    // context / initialized SDL video, neither of which exist yet when
    // Engine's singleton itself is first constructed).
    mRenderInterface = std::make_unique<RenderInterface_GL3>();
    mSystemInterface = std::make_unique<SystemInterface_SDL>();

    Rml::SetSystemInterface(mSystemInterface.get());
    Rml::SetRenderInterface(mRenderInterface.get());

    mSystemInterface->SetWindow(pWindow);

    Rml::Initialise();

    int tWidth = 1;
    int tHeight = 1;
    SDL_GetWindowSizeInPixels(pWindow, &tWidth, &tHeight);

    mLastKnownSize = Rml::Vector2i(tWidth, tHeight);
    mRenderInterface->SetViewport(tWidth, tHeight);

    mContext = Rml::CreateContext("main", mLastKnownSize, mRenderInterface.get());
    updateScale();

    // Same vendored font asset ImGui already uses (src/game/assets/fonts) -
    // no new font file needed.
    std::string tFontPath = FileManager::createPath(FileManager::getAssetPath(), "fonts/Roboto-Medium.ttf");
    Rml::LoadFontFace(tFontPath);
}

void RmlUiLayer::deinitialize()
{
    // Order matters: Rml::Shutdown() destroys the context and releases
    // RmlUi's own GL resources through the render interface, so the
    // interfaces must still be alive (and the GL context still current)
    // when it runs - only reset them afterward.
    Rml::Shutdown();

    Rml::SetSystemInterface(nullptr);
    Rml::SetRenderInterface(nullptr);

    mRenderInterface.reset();
    mSystemInterface.reset();

    RmlGL3::Shutdown();
}

void RmlUiLayer::update()
{
    if (!mContext)
    {
        return;
    }

    int tWidth = 1;
    int tHeight = 1;
    SDL_GetWindowSizeInPixels(mWindow, &tWidth, &tHeight);

    // Physical framebuffer size, matching what WorldScene standardized on
    // for its own screen-space math after this session's earlier DPI
    // mismatch bug - kept consistent so RmlUi doesn't reintroduce it.
    Rml::Vector2i tCurrentSize(tWidth, tHeight);
    if (tCurrentSize != mLastKnownSize)
    {
        mLastKnownSize = tCurrentSize;
        mContext->SetDimensions(tCurrentSize);
        mRenderInterface->SetViewport(tWidth, tHeight);
        updateScale();
    }

    mContext->Update();
}

void RmlUiLayer::render()
{
    if (!mContext)
    {
        return;
    }

    mRenderInterface->BeginFrame();
    mContext->Render();
    mRenderInterface->EndFrame();
}

bool RmlUiLayer::handleEvent(SDL_Event* pEvent)
{
    if (!mContext)
    {
        return false;
    }

    // Clear a STALE focus - an element RmlUi still considers "focused" even
    // though it (or an ancestor) has since become display:none, e.g. a chat
    // window's message input right after that window gets minimized
    // (data-if just toggles display:none, it never actually removes the
    // element or blurs it - confirmed earlier this session against RmlUi
    // 6.1's own DataViewIf::Update). This has to happen BEFORE
    // InputEventHandler below, not after: RmlSDL routes key/text events
    // straight to Context::GetFocusElement() internally regardless of that
    // element's visibility, so a stale focus keeps having characters typed
    // into it (confirmed empirically in scratchpad/rmlrepro - even once the
    // check further down correctly stopped reporting the event as
    // "consumed", the keystroke had already been delivered into the hidden
    // input by the time that check even ran). Checking after the fact can't
    // undo an internal dispatch that already happened.
    if ((pEvent->type == SDL_EVENT_KEY_DOWN || pEvent->type == SDL_EVENT_KEY_UP || pEvent->type == SDL_EVENT_TEXT_INPUT))
    {
        Rml::Element* tStaleFocus = mContext->GetFocusElement();
        if (tStaleFocus && !tStaleFocus->IsVisible(true))
        {
            tStaleFocus->Blur();
        }
    }

    bool tWasConsumed = !RmlSDL::InputEventHandler(mContext, mWindow, *pEvent);

    // RmlSDL's InputEventHandler routes character insertion through
    // ProcessTextInput rather than ProcessKeyDown, so whether a plain WASD
    // keydown gets reported as "consumed" while a text field has focus
    // isn't guaranteed the way ImGui's blanket WantCaptureKeyboard was -
    // this class of bug (character moving while typing in chat) already
    // happened once against ImGui this session, so it's closed here
    // proactively rather than waiting to rediscover it against RmlUi's
    // input fields: force-consume every key/text event while a text-editable
    // element has focus, regardless of what InputEventHandler reported.
    // IsVisible(true) here is now mostly a defensive backstop - the Blur()
    // above already clears the one case that used to reach it - but costs
    // nothing to keep.
    if (!tWasConsumed &&
        (pEvent->type == SDL_EVENT_KEY_DOWN || pEvent->type == SDL_EVENT_KEY_UP || pEvent->type == SDL_EVENT_TEXT_INPUT))
    {
        Rml::Element* tFocus = mContext->GetFocusElement();

        if (tFocus && (tFocus->GetTagName() == "input" || tFocus->GetTagName() == "textarea") && tFocus->IsVisible(true))
        {
            tWasConsumed = true;
        }
    }

    return tWasConsumed;
}

void RmlUiLayer::registerGeneratedImage(const std::string& pName, int pWidth, int pHeight, std::vector<uint8_t> pRgba)
{
    if (mRenderInterface)
    {
        // Opaque pixels are already premultiplied, which is what RmlUi wants.
        mRenderInterface->SetGeneratedImage(pName, Rml::Vector2i(pWidth, pHeight),
                                            Rml::Vector<Rml::byte>(pRgba.begin(), pRgba.end()));
    }
}

Rml::ElementDocument* RmlUiLayer::loadDocument(const std::string& pRelativePath)
{
    if (!mContext)
    {
        return nullptr;
    }

    std::string tFullPath = FileManager::createPath(FileManager::getAssetPath(), pRelativePath);
    return mContext->LoadDocument(tFullPath);
}

Rml::ElementDocument* RmlUiLayer::loadDocumentFromTemplate(const std::string& pRelativePath, const std::string& pModelPlaceholder, const std::string& pModelName)
{
    if (!mContext)
    {
        return nullptr;
    }

    std::string tFullPath = FileManager::createPath(FileManager::getAssetPath(), pRelativePath);

    std::ifstream tFile(tFullPath);
    if (!tFile.is_open())
    {
        SDL_Log("RmlUiLayer: failed to open template '%s'", tFullPath.c_str());
        return nullptr;
    }

    std::stringstream tBuffer;
    tBuffer << tFile.rdbuf();
    std::string tContent = tBuffer.str();

    size_t tPos = tContent.find(pModelPlaceholder);
    if (tPos != std::string::npos)
    {
        tContent.replace(tPos, pModelPlaceholder.size(), pModelName);
    }

    // Unlike LoadDocument(path) - which internally goes through
    // StreamFile::Open and pre-sanitizes exactly this the same way -
    // LoadDocumentFromMemory's source_url is handed straight to Rml::URL
    // with no such treatment. Rml::URL::SetURL() treats the first ':' as a
    // scheme delimiter and requires "://" right after it; a raw Windows
    // path's drive-letter colon ("C:\...") fails that check and asserts
    // (RMLUI_ASSERT(SetURL(_url)), URL.cpp) - hit and confirmed against
    // RmlUi's own vcpkg buildtree source during this task. Replacing ':'
    // with '|' is the same workaround StreamFile::Open itself uses; it only
    // affects this cosmetic/debug "source" string (relative <link> href
    // resolution still works identically), never the real path used to
    // actually open the file above.
    std::string tSourceUrl = tFullPath;
    std::replace(tSourceUrl.begin(), tSourceUrl.end(), ':', '|');

    return mContext->LoadDocumentFromMemory(tContent, tSourceUrl);
}

Rml::Context* RmlUiLayer::getContext() const
{
    return mContext;
}

float RmlUiLayer::getScale() const
{
    return mContext ? mContext->GetDensityIndependentPixelRatio() : 1.0f;
}

void RmlUiLayer::updateScale()
{
    float tScale = std::min(mLastKnownSize.x / kDesignWidth, mLastKnownSize.y / kDesignHeight);
    mContext->SetDensityIndependentPixelRatio(std::clamp(tScale, 0.5f, 2.0f));
}

Rml::Vector2f RmlUiLayer::clampDocumentToScreen(Rml::ElementDocument* pDocument, Rml::Element* pWindow, Rml::Vector2f pWanted) const
{
    if (!mContext || !pDocument || !pWindow)
    {
        return pWanted;
    }

    Rml::Vector2f tOffset(pWindow->GetAbsoluteLeft() - pDocument->GetAbsoluteLeft(),
                          pWindow->GetAbsoluteTop() - pDocument->GetAbsoluteTop());
    Rml::Vector2f tSize(pWindow->GetOffsetWidth(), pWindow->GetOffsetHeight());
    Rml::Vector2f tScreen(static_cast<float>(mContext->GetDimensions().x), static_cast<float>(mContext->GetDimensions().y));

    return Rml::Vector2f(std::clamp(pWanted.x, -tOffset.x, std::max(-tOffset.x, tScreen.x - tOffset.x - tSize.x)),
                         std::clamp(pWanted.y, -tOffset.y, std::max(-tOffset.y, tScreen.y - tOffset.y - tSize.y)));
}
