#ifndef LAKOT_RMLUILAYER_H
#define LAKOT_RMLUILAYER_H

#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include "backend/RmlUi_Renderer_GL3.h"
#include "backend/RmlUi_Platform_SDL.h"

namespace lakot
{

// RmlUi's counterpart to GuiLayer - owns the Rml::Context and both
// interfaces RmlUi requires an application to provide (render + system),
// and drives its per-frame update/render and SDL event translation. RmlUi
// renders every player-facing document (login, later chat/inventory/skills);
// GuiLayer/ImGui remains untouched for developer/debug tooling only.
class RmlUiLayer
{
public:
    RmlUiLayer() = default;
    ~RmlUiLayer() = default;

    void initialize(SDL_Window* pWindow, SDL_GLContext pGLContext);
    void deinitialize();

    // Called once per rendered frame (not the fixed-timestep loop - RmlUi
    // expects Update()/Render() at display rate).
    void update();
    void render();

    // Translates the SDL event into the matching Rml::Context::Process*()
    // call and returns true if RmlUi's own UI consumed it. RmlSDL's
    // InputEventHandler returns true when the event is still propagating
    // (i.e. RmlUi did NOT want it), so the result is inverted here to match
    // the "did the UI capture this input" convention Engine::eventHandler
    // needs to gate WorldScene/camera input on.
    bool handleEvent(SDL_Event* pEvent);

    // Resolves pRelativePath against FileManager::getAssetPath() (e.g.
    // "ui/login.rml") and loads it as a new document on this context. The
    // returned Rml::ElementDocument* is used directly via its own
    // Show()/Hide() - no extra Panel-style wrapper, RmlUi's document already
    // is that abstraction.
    Rml::ElementDocument* loadDocument(const std::string& pRelativePath);

    // Makes an image available to documents as src="lakot-generated/<pName>".
    // pRgba is width * height * 4 bytes, fully opaque.
    void registerGeneratedImage(const std::string& pName, int pWidth, int pHeight, std::vector<uint8_t> pRgba);

    // For documents that need more than one simultaneous instance (chat: one
    // per whisper conversation) - Rml::DataModelHandle names are global to
    // this shared Context, so every instance needs its own unique
    // data-model name, which the .rml file itself can't parameterize.
    // Reads pRelativePath's text, replaces pModelPlaceholder (the literal
    // string the template ships with, e.g. "__CHAT_MODEL__") with
    // pModelName, and loads the patched text via LoadDocumentFromMemory
    // with the template's real path as the source URL, so its own
    // <link href="theme.rcss"/> (and similar relative references) still
    // resolve correctly.
    Rml::ElementDocument* loadDocumentFromTemplate(const std::string& pRelativePath, const std::string& pModelPlaceholder, const std::string& pModelName);

    Rml::Context* getContext() const;

    float getScale() const;

    Rml::Vector2f clampDocumentToScreen(Rml::ElementDocument* pDocument, Rml::Element* pWindow, Rml::Vector2f pWanted) const;

private:
    static constexpr float kDesignWidth = 1920.0f;
    static constexpr float kDesignHeight = 1080.0f;

    SDL_Window* mWindow{nullptr};

    // Heap-allocated and constructed inside initialize() rather than as
    // plain members - both constructors do real work (RenderInterface_GL3
    // compiles its shader programs via live GL calls; SystemInterface_SDL
    // creates SDL system cursors) that requires a current GL context/loaded
    // glad and SDL_Init(SDL_INIT_VIDEO) to have already happened. Engine is
    // a Meyer's singleton constructed on its very first getInstance() call
    // in main.cpp, before either of those exist - a plain value member here
    // would run that work far too early and crash (this was hit and fixed
    // during Phase 1 verification: access violation at address 0 - a null
    // GL function pointer call - from RenderInterface_GL3's constructor).
    std::unique_ptr<RenderInterface_GL3> mRenderInterface;
    std::unique_ptr<SystemInterface_SDL> mSystemInterface;

    Rml::Context* mContext{nullptr};

    Rml::Vector2i mLastKnownSize{0, 0};

    void updateScale();
};

}

#endif
