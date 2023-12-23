#include "GUI.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "platform/Platform.h"

#include "core/helper/window/WindowManager.h"

#include "Settings.h"

GUI::GUI()
{
    mVisibility = false;
}

void GUI::initialize()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(reinterpret_cast<GLFWwindow*>(WindowManager::getInstance()->getWindow()), true);
    ImGui_ImplOpenGL3_Init(LAKOT_GLSL_VERSION);
}

void GUI::render()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Settings");

    bool tIsMouseMovementEnabled = Settings::getInstance()->getIsMouseMovementEnabled();
    ImGui::Checkbox("Mouse Movement Enabled", &tIsMouseMovementEnabled);
    Settings::getInstance()->setIsMouseMovementEnabled(tIsMouseMovementEnabled);

    static char testInput[128] = "Placeholder";
    ImGui::InputText("Test Input", testInput, IM_ARRAYSIZE(testInput));

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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
    auto& io = ImGui::GetIO();

    if (io.WantCaptureMouse)
    {
        return true;
    }

    return false;
}

bool GUI::isKeyboardEventOnGUI()
{
    auto& io = ImGui::GetIO();

    if (io.WantTextInput || io.WantCaptureKeyboard)
    {
        return true;
    }

    return false;
}
