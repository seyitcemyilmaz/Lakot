#include "GuiLayer.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

using namespace lakot;

void GuiLayer::initialize(SDL_Window* pWindow, SDL_GLContext pGLContext)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& tIO = ImGui::GetIO();

    tIO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    tIO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    tIO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& tStyle = ImGui::GetStyle();
    if (tIO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        tStyle.WindowRounding = 0.0f;
        tStyle.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplSDL3_InitForOpenGL(pWindow, pGLContext);

#if defined(SDL_PLATFORM_IOS) || defined(SDL_PLATFORM_ANDROID)
    ImGui_ImplOpenGL3_Init("#version 300 es");
#elif defined(__APPLE__)
    ImGui_ImplOpenGL3_Init("#version 410");
#else
    ImGui_ImplOpenGL3_Init("#version 460");
#endif
}

void GuiLayer::deinitialize()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void GuiLayer::begin()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGuiDockNodeFlags tDockFlags = ImGuiDockNodeFlags_PassthruCentralNode;

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), tDockFlags);
}

void GuiLayer::end()
{
    for (auto& tPanel : mPanels)
    {
        if (tPanel->isOpen)
        {
            if (tPanel->isFullScreen)
            {
                ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);
                ImGui::SetNextWindowViewport(viewport->ID);
            }
            else if (tPanel->centerWindow)
            {
                ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            }

            // Pencereyi başlat
            // Not: Tam ekran pencerelerde padding'i kaldırmak isteyebilirsin
            if (tPanel->isFullScreen) ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

            if (ImGui::Begin(tPanel->name.c_str(), &tPanel->isOpen, tPanel->windowFlags))
            {
                tPanel->render();
            }
            ImGui::End();

            if (tPanel->isFullScreen) ImGui::PopStyleVar();
        }
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& tIO = ImGui::GetIO();
    if (tIO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* tBackupWindow = SDL_GL_GetCurrentWindow();
        SDL_GLContext tBackupContext = SDL_GL_GetCurrentContext();

        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();

        SDL_GL_MakeCurrent(tBackupWindow, tBackupContext);
    }
}

bool GuiLayer::handleEvent(SDL_Event* pEvent)
{
    ImGui_ImplSDL3_ProcessEvent(pEvent);

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantCaptureMouse)
    {
        if (pEvent->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
            pEvent->type == SDL_EVENT_MOUSE_BUTTON_UP ||
            pEvent->type == SDL_EVENT_MOUSE_WHEEL ||
            pEvent->type == SDL_EVENT_MOUSE_MOTION)
        {
            return true;
        }
    }

    if (io.WantCaptureKeyboard)
    {
        if (pEvent->type == SDL_EVENT_KEY_DOWN ||
            pEvent->type == SDL_EVENT_KEY_UP ||
            pEvent->type == SDL_EVENT_TEXT_INPUT)
        {
            return true;
        }
    }

    return false;
}

void GuiLayer::addPanel(std::shared_ptr<Panel> pPanel)
{
    mPanels.push_back(pPanel);
}

void GuiLayer::clearPanels()
{
    mPanels.clear();
}
