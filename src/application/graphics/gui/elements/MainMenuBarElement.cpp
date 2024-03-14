#include "MainMenuBarElement.h"

#include <spdlog/spdlog.h>

#include <imgui.h>

MainMenuBarElement::~MainMenuBarElement()
{

}

MainMenuBarElement::MainMenuBarElement()
    : IGuiElement()
{

}

void MainMenuBarElement::render(void* pObject)
{
    (void)pObject;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Create", "Ctrl+X"))
            {
                spdlog::info("create basildi");
            }
            if (ImGui::MenuItem("Open", "Ctrl+O"))
            {

            }
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {

            }
            if (ImGui::MenuItem("Save as.."))
            {

            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
