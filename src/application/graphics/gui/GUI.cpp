#include "GUI.h"

#include <spdlog/spdlog.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "platform/Platform.h"

#include "core/helper/window/WindowManager.h"

#include "application/Settings.h"

#include "elements/MainMenuBarElement.h"
#include "elements/CameraElement.h"

GUI::~GUI()
{

}

GUI::GUI()
{
    mVisibility = false;
}

void GUI::initialize()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& tIO = ImGui::GetIO();
    tIO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;      // Enable Keyboard Controls
    tIO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;       // Enable Gamepad Controls
    tIO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;          // Enable Docking
    tIO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;        // Enable Multi-Viewport / Platform Windows

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(reinterpret_cast<GLFWwindow*>(WindowManager::getInstance()->getWindow()->getWindowInstance()), true);
    ImGui_ImplOpenGL3_Init(LAKOT_GLSL_VERSION);

    MainMenuBarElement* tMainMenuBarElement = new MainMenuBarElement();
    addGuiElement(tMainMenuBarElement);

    CameraElement* tCameraElement = new CameraElement();
    addGuiElement(tCameraElement);
}

void GUI::deinitialize()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void GUI::render(Scene* pScene)
{
    preRenderPhase();

    for (unsigned int i = 0; i < mGuiElements.size(); i++)
    {
        mGuiElements[i]->render(pScene);
    }

    ImGui::Begin("Settings");

    bool tIsMouseMovementEnabled = Settings::getInstance()->getIsMouseMovementEnabled();
    ImGui::Checkbox("Mouse Movement Enabled", &tIsMouseMovementEnabled);
    Settings::getInstance()->setIsMouseMovementEnabled(tIsMouseMovementEnabled);

    static char testInput[128] = "Placeholder";
    ImGui::InputText("Test Input", testInput, IM_ARRAYSIZE(testInput));

    ImGui::End();

    postRenderPhase();
}

void GUI::setVisibility(bool pVisibility)
{
    mVisibility = pVisibility;
}

bool GUI::getVisibility()
{
    return mVisibility;
}

bool GUI::isMouseEventOnGUI()
{
    ImGuiIO& tIO = ImGui::GetIO();

    return tIO.WantCaptureMouse || tIO.WantCaptureMouseUnlessPopupClose;
}

bool GUI::isKeyboardEventOnGUI()
{
    ImGuiIO& tIO = ImGui::GetIO();

    return tIO.WantTextInput || tIO.WantCaptureKeyboard;
}

void GUI::addGuiElement(IGuiElement* pGuiElement)
{
    mGuiElements.push_back(pGuiElement);
}

void GUI::preRenderPhase()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiWindowFlags tWindowFlags = ImGuiWindowFlags_MenuBar |
                                    ImGuiWindowFlags_NoDocking |
                                    ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus |
                                    ImGuiWindowFlags_NoBackground;

    ImGuiViewport* tImGuiViewPort = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(tImGuiViewPort->Pos);
    ImGui::SetNextWindowSize(tImGuiViewPort->Size);
    ImGui::SetNextWindowViewport(tImGuiViewPort->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("MainWindow", nullptr, tWindowFlags);

    ImGui::PopStyleVar(3);

    ImGuiID tDockSpaceId = ImGui::GetID("MainWindowDockSpace");
    ImGui::DockSpace(tDockSpaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
}

void GUI::postRenderPhase()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}
